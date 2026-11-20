/*
 * Simulator.cpp
 */

#include "Simulator.h"
#include "Debug.h"
#include "Storage.h"
#include "tracy/Tracy.hpp"
#include "utils/StorageHelper.h"
#include <charconv>

using namespace std::chrono_literals;

namespace Sim
{
	namespace
	{
		/** The number a line sent starts with (`N12 M409`), if it has one */
		std::optional<int64_t> lineNumber(std::string_view line)
		{
			const size_t start = line.find_first_not_of(" \t");
			if (start == std::string_view::npos || (line[start] != 'N' && line[start] != 'n'))
			{
				return std::nullopt;
			}
			int64_t number = 0;
			const char* first = line.data() + start + 1;
			const auto [end, error] = std::from_chars(first, line.data() + line.size(), number);
			return error == std::errc() && end != first ? std::optional(number) : std::nullopt;
		}
	} // namespace

	namespace
	{
		/** The configuration, kept with the screen's settings; the default preset until one is chosen */
		constexpr StorageKey<Config, Config (*)()> ID_SIMULATOR = {"simulator", &defaultConfig};

		/** How often the thread lets time pass and answers what is due */
		constexpr std::chrono::milliseconds TICK = 50ms;

		/** At most this many rounds of replies leading to requests in one process() */
		constexpr int MAX_ROUNDS = 20;
	} // namespace

	Simulator& Simulator::get()
	{
		static Simulator simulator;
		return simulator;
	}

	Simulator::Simulator()
		: m_machine(StorageHelper::getData(ID_SIMULATOR))
	{
		m_machine.setNoise(m_behaviour.noise);
	}

	Simulator::~Simulator()
	{
		stop();
	}

	bool Simulator::connect(Receiver receiver)
	{
		{
			std::lock_guard lock(m_lock);
			if (!m_behaviour.pluggedIn)
			{
				LOG_WARN("The simulated Duet is unplugged");
				return false;
			}
			clearInput();
			m_receiver = std::move(receiver);
			start();
		}
		LOG_INFO("Connected to the simulated Duet");
		return true;
	}

	void Simulator::disconnect()
	{
		{
			std::lock_guard lock(m_lock);
			m_receiver = nullptr;
			clearInput();
		}
		stop();
	}

	bool Simulator::connected() const
	{
		std::lock_guard lock(m_lock);
		return m_receiver != nullptr;
	}

	void Simulator::send(std::string_view bytes)
	{
		std::lock_guard lock(m_lock);
		if (!m_receiver || bytes.empty())
		{
			return;
		}
		const auto due = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_behaviour.replyDelayMs);
		m_received.push_back({.bytes = std::string(bytes), .due = due});
	}

	Config Simulator::config() const
	{
		std::lock_guard lock(m_lock);
		return m_machine.config();
	}

	void Simulator::setConfig(Config config)
	{
		std::lock_guard lock(m_lock);
		m_machine.reconfigure(config);
		save();
	}

	Behaviour Simulator::behaviour() const
	{
		std::lock_guard lock(m_lock);
		return m_behaviour;
	}

	void Simulator::setBehaviour(Behaviour behaviour)
	{
		std::lock_guard lock(m_lock);
		m_behaviour = behaviour;
		m_machine.setNoise(behaviour.noise);
		if (!behaviour.responding || !behaviour.pluggedIn)
		{
			clearInput();
		}
	}

	void Simulator::forceStatus(std::optional<std::string> status)
	{
		std::lock_guard lock(m_lock);
		m_machine.forceStatus(std::move(status));
	}

	std::optional<std::string> Simulator::forcedStatus() const
	{
		std::lock_guard lock(m_lock);
		return m_machine.forcedStatus();
	}

	void Simulator::showMessageBox(int32_t mode)
	{
		std::lock_guard lock(m_lock);
		m_machine.showMessageBox(mode);
	}

	void Simulator::reset()
	{
		disconnect();
		std::lock_guard lock(m_lock);
		m_behaviour = Behaviour{};
		m_machine = Machine(StorageHelper::getData(ID_SIMULATOR));
		m_machine.setNoise(m_behaviour.noise);
	}

	void Simulator::setManual(bool manual)
	{
		if (manual)
		{
			stop();
		}
		std::lock_guard lock(m_lock);
		m_manual = manual;
		if (!manual && m_receiver)
		{
			start();
		}
	}

	void Simulator::process()
	{
		for (int round = 0; round < MAX_ROUNDS; round++)
		{
			std::vector<std::string> replies;
			{
				std::lock_guard lock(m_lock);
				if (m_received.empty())
				{
					return;
				}
				replies = answer(std::chrono::steady_clock::time_point::max());
			}
			deliver(replies);
		}
		LOG_WARN("The simulated Duet is still answering after {:d} rounds", MAX_ROUNDS);
	}

	void Simulator::advance(std::chrono::milliseconds time)
	{
		std::lock_guard lock(m_lock);
		m_machine.advance(std::chrono::duration<double>(time).count());
	}

	void Simulator::run()
	{
		tracy::SetThreadName("Simulator");
		auto last = std::chrono::steady_clock::now();
		while (m_running)
		{
			std::this_thread::sleep_for(TICK);
			const auto now = std::chrono::steady_clock::now();
			std::vector<std::string> replies;
			{
				std::lock_guard lock(m_lock);
				m_machine.advance(std::chrono::duration<double>(now - last).count());
				replies = answer(now);
			}
			last = now;
			deliver(replies);
		}
	}

	std::vector<std::string> Simulator::answer(std::chrono::steady_clock::time_point now)
	{
		std::vector<std::string> replies;
		if (!m_behaviour.responding || !m_behaviour.pluggedIn)
		{
			clearInput();
			return replies;
		}
		while (!m_received.empty() && m_received.front().due <= now)
		{
			m_input += m_received.front().bytes;
			m_received.pop_front();
		}
		// As RepRapFirmware 3.6.2 on does, a reply is numbered with the line it answers, so the screen can match it
		const auto numbered = [&replies](std::vector<Json> answers, std::optional<int64_t> number)
		{
			for (Json& reply : answers)
			{
				if (number && reply.is_object())
				{
					reply["line"] = *number;
				}
				// A file written to it may hold anything: bytes that aren't UTF-8 are replaced, not thrown at
				replies.push_back(reply.dump(-1, ' ', false, Json::error_handler_t::replace) + "\n");
			}
		};
		while (true)
		{
			if (m_machine.receivingFile())
			{
				std::vector<Json> answers = m_machine.receiveFile(m_input);
				if (m_machine.receivingFile())
				{
					break; // more of the file to come
				}
				numbered(std::move(answers), m_fileLine); // an error with the file names M559's line
				m_fileLine.reset();
				continue;
			}
			const size_t end = m_input.find('\n');
			if (end == std::string::npos)
			{
				break; // the rest of the line to come
			}
			std::string line = m_input.substr(0, end);
			m_input.erase(0, end + 1);
			if (line.ends_with('\r'))
			{
				line.pop_back();
			}
			if (line.empty())
			{
				continue;
			}
			const std::optional<int64_t> number = lineNumber(line);
			numbered(m_machine.execute(line), number);
			if (m_machine.receivingFile())
			{
				m_fileLine = number;
			}
		}
		if (m_machine.takeConfigChange())
		{
			save();
		}
		return replies;
	}

	void Simulator::deliver(const std::vector<std::string>& replies)
	{
		Receiver receiver;
		{
			std::lock_guard lock(m_lock);
			receiver = m_receiver;
		}
		if (!receiver)
		{
			return;
		}
		for (const std::string& reply : replies)
		{
			LOG_VERBOSE("Simulated Duet replies: {:s}", reply);
			receiver(reply);
		}
	}

	void Simulator::clearInput()
	{
		m_received.clear();
		m_input.clear();
		m_fileLine.reset();
		m_machine.abandonFile();
	}

	void Simulator::save() const
	{
		StorageHelper::setData(ID_SIMULATOR, m_machine.config());
	}

	void Simulator::start()
	{
		if (m_manual || m_running)
		{
			return;
		}
		if (m_thread.joinable())
		{
			m_thread.join();
		}
		m_running = true;
		m_thread = std::thread(&Simulator::run, this);
	}

	void Simulator::stop()
	{
		m_running = false;
		if (!m_thread.joinable())
		{
			return;
		}
		if (m_thread.get_id() == std::this_thread::get_id())
		{
			// Disconnected by a reply it sent: the loop ends by itself
			m_thread.detach();
			return;
		}
		m_thread.join();
	}
} // namespace Sim

/*
 * Simulator.cpp
 */

#include "Simulator.h"
#include "Debug.h"
#include "Storage.h"
#include "tracy/Tracy.hpp"
#include "utils/StorageHelper.h"

using namespace std::chrono_literals;

namespace Sim
{
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
			m_requests.clear();
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
			m_requests.clear();
		}
		stop();
	}

	bool Simulator::connected() const
	{
		std::lock_guard lock(m_lock);
		return m_receiver != nullptr;
	}

	void Simulator::send(std::string_view gcode)
	{
		std::lock_guard lock(m_lock);
		if (!m_receiver)
		{
			return;
		}
		const auto due = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_behaviour.replyDelayMs);
		size_t start = 0;
		while (start < gcode.size())
		{
			const size_t end = gcode.find('\n', start);
			const std::string_view line =
				gcode.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
			if (!line.empty())
			{
				m_requests.push_back({.line = std::string(line), .due = due});
			}
			if (end == std::string_view::npos)
			{
				break;
			}
			start = end + 1;
		}
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
			m_requests.clear();
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
				if (m_requests.empty())
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
			m_requests.clear();
			return replies;
		}
		while (!m_requests.empty() && m_requests.front().due <= now)
		{
			const std::string line = std::move(m_requests.front().line);
			m_requests.pop_front();
			for (const Json& reply : m_machine.execute(line))
			{
				replies.push_back(reply.dump() + "\n");
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

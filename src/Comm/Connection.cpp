/*
 * Connection.cpp
 */

#include "Connection.h"
#include "Comm/FileInfoCache.h"
#include "Comm/FileTransfer.h"
#include "Comm/LineFramer.h"
#include "Comm/Links/DsfLink.h"
#include "Comm/Links/HttpLink.h"
#include "Comm/Links/SerialLink.h"
#include "Comm/Ports/UartPort.h"
#include "Comm/Ports/UsbPort.h"
#include "Comm/Responses.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Followers.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tree/Patch.h"
#include "ObjectModel/Tree/Tree.h"
#include "ObjectModel/Utils.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "nameof.hpp"
#include "tracy/Tracy.hpp"
#include "utils/StorageHelper.h"
#if DUET_SIMULATOR
#  include "Comm/Ports/SimulatorPort.h"
#endif

namespace Comm
{
	namespace
	{
		/** The longest the connection's thread sleeps without looking at what is due (a port that has gone says
		 * nothing) */
		constexpr std::chrono::milliseconds longestSleep{1000};

		/** Act on a control command, which the firmware sends to update or restart the screen */
		void controlCommand(std::string_view command)
		{
			ZoneScoped;
			if (command == "reset")
			{
				Restart(); // Does not return
			}
			else if (command == "eraseAndReset")
			{
				EraseAndRestart(); // Does not return
			}
			else
			{
				LOG_WARN("Unknown control command \"{:s}\"", command);
			}
		}

		/** Tell the UI how writing a file is going */
		void reportTransfer(FileTransfer transfer)
		{
			ZoneScoped;
			Model::get().post<EventType::FileTransfer>(std::move(transfer));
		}

		/** A handler that only notes a failure, for a request whose asker doesn't wait for the answer */
		Handler<void> logFailure(std::string what)
		{
			ZoneScoped;
			return [what = std::move(what)](std::expected<void, ReplyError> result)
			{
				ZoneScopedN("logFailure_cb");
				if (!result)
				{
					LOG_WARN("{:s} failed: {:s}", what, result.error().message);
				}
			};
		}
	} // namespace

	Connection& Connection::get()
	{
		ZoneScoped;
		static Connection s_connection;
		return s_connection;
	}

	Connection::Connection()
		: m_pollInterval(DEFAULT_PRINTER_POLL_INTERVAL)
	{
		ZoneScoped;
	}

	Connection::~Connection()
	{
		ZoneScoped;
		// Only the thread: the rest of the program may have gone already, so the link goes without telling anyone
		stopThread();
	}

	void Connection::stopThread()
	{
		ZoneScoped;
		m_running = false;
		m_queueChanged.notify_all();
		if (!m_thread.joinable())
		{
			return;
		}
		if (m_thread.get_id() == std::this_thread::get_id())
		{
			m_thread.detach(); // the program is ending from the connection's own thread (a reset the firmware sent)
		}
		else
		{
			m_thread.join();
		}
	}

	void Connection::start()
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_settingsMutex);
			m_ipAddress = StorageHelper::getData(ID_DUET_IP_ADDRESS);
			m_password = StorageHelper::getData(ID_DUET_PASSWORD);
		}
		m_pollInterval = std::max(StorageHelper::getData(ID_DUET_POLL_INTERVAL), MIN_PRINTER_POLL_INTERVAL);
		CommunicationType type = StorageHelper::getData(ID_DUET_COMMUNICATION_TYPE);
#if !DUET_SIMULATOR
		if (type == CommunicationType::simulator)
		{
			type = CommunicationType::usb; // a build without the simulated Duet
		}
#endif
		m_type = type;
		post([this] { m_retryAt = Clock::now(); });
		if (!m_manual && !m_running)
		{
			m_running = true;
			m_thread = std::thread(&Connection::run, this);
		}
	}

	void Connection::stop()
	{
		ZoneScoped;
		stopThread();
		{
			std::lock_guard lock(m_queueMutex);
			m_queue.clear();
			m_timers.clear();
		}
		disconnect();
		m_retryAt.reset();
	}

	std::string_view Connection::typeName() const
	{
		ZoneScoped;
		return communicationTypeNames[static_cast<size_t>(type())];
	}

	void Connection::setType(CommunicationType type)
	{
		ZoneScoped;
#if !DUET_SIMULATOR
		if (type == CommunicationType::simulator)
		{
			LOG_WARN("This build has no simulated Duet");
			return;
		}
#endif
		LOG_INFO("Connecting by {:s}", nameof::nameof_enum(type));
		m_type = type;
		StorageHelper::setData(ID_DUET_COMMUNICATION_TYPE, type);
		reconnect();
	}

	void Connection::setPollInterval(std::chrono::milliseconds interval)
	{
		ZoneScoped;
		if (interval < MIN_PRINTER_POLL_INTERVAL)
		{
			LOG_WARN("Poll interval too low, setting to {}", MIN_PRINTER_POLL_INTERVAL);
			interval = MIN_PRINTER_POLL_INTERVAL;
		}
		m_pollInterval = interval;
		StorageHelper::setData(ID_DUET_POLL_INTERVAL, interval);
		post(
			[this, interval]
			{
				ZoneScopedN("setPollInterval_cb");
				if (m_sync)
				{
					m_sync->setPollInterval(interval);
				}
			});
	}

	std::string Connection::ipAddress() const
	{
		ZoneScoped;
		std::lock_guard lock(m_settingsMutex);
		return m_ipAddress;
	}

	void Connection::setIpAddress(std::string_view address)
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_settingsMutex);
			if (m_ipAddress == address)
			{
				return;
			}
			m_ipAddress = address;
		}
		LOG_INFO("Setting IP address to {:s}", address);
		StorageHelper::setData(ID_DUET_IP_ADDRESS, address);
		if (type() == CommunicationType::network)
		{
			reconnect();
		}
	}

	std::string Connection::password() const
	{
		ZoneScoped;
		std::lock_guard lock(m_settingsMutex);
		return m_password;
	}

	void Connection::setPassword(std::string_view password)
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_settingsMutex);
			m_password = password;
		}
		StorageHelper::setData(ID_DUET_PASSWORD, password);
		if (type() == CommunicationType::network)
		{
			reconnect();
		}
	}

	void Connection::reconnect()
	{
		ZoneScoped;
		post(
			[this]
			{
				ZoneScopedN("reconnect_cb");
				disconnect();
				m_retryAt = Clock::now();
			});
	}

	void Connection::sendGcode(std::string_view gcode)
	{
		ZoneScoped;
		post(
			[this, gcode = std::string(gcode)]
			{
				ZoneScopedN("sendGcode_cb");
				if (m_state != State::connected || !m_link)
				{
					LOG_DBG("Not connected to the printer, not sending: {:s}", gcode);
					return;
				}
				m_link->sendGcode(gcode);
			});
	}

	void Connection::emergencyStop()
	{
		ZoneScoped;
		post(
			[this]
			{
				ZoneScopedN("emergencyStop_cb");
				if (!m_link)
				{
					LOG_WARN("Not connected to the printer: it can't be stopped");
					return;
				}
				m_link->emergencyStop();
				LOG_WARN("Emergency stop sent");
			});
	}

	template <typename T, typename Ask>
	void Connection::ask(Handler<T> handler, Ask ask)
	{
		ZoneScoped;
		post(
			[this, handler = std::move(handler), ask = std::move(ask)]() mutable
			{
				ZoneScopedN("ask_cb");
				if (m_state != State::connected || !m_link)
				{
					handler(std::unexpected(ReplyError{.message = "not connected to the printer"}));
					return;
				}
				ask(*m_link, std::move(handler));
			});
	}

	void Connection::listFiles(std::string dir, size_t first, Handler<FileList> handler)
	{
		ZoneScoped;
		ask(std::move(handler),
			[dir = std::move(dir), first](Link& link, Handler<FileList> answer)
			{ link.listFiles(dir, first, std::move(answer)); });
	}

	void Connection::fileInfo(std::string file, Handler<FileInfo> handler)
	{
		ZoneScoped;
		ask(std::move(handler),
			[file = std::move(file)](Link& link, Handler<FileInfo> answer) { link.fileInfo(file, std::move(answer)); });
	}

	void Connection::thumbnail(std::string file, uint32_t offset, Handler<FileChunk> handler)
	{
		ZoneScoped;
		ask(std::move(handler),
			[file = std::move(file), offset](Link& link, Handler<FileChunk> answer)
			{ link.thumbnail(file, offset, std::move(answer)); });
	}

	void Connection::readFile(std::string file, Handler<std::string> handler)
	{
		ZoneScoped;
		ask(std::move(handler),
			[file = std::move(file)](Link& link, Handler<std::string> answer)
			{ link.readFile(file, std::move(answer)); });
	}

	void Connection::writeFile(std::string file, FileContents contents, Handler<void> handler)
	{
		ZoneScoped;
		if (!handler)
		{
			handler = logFailure(fmt::format("Writing {:s}", file));
		}
		const size_t total = contents.size();
		// The UI is told it ended, however it went
		Handler<void> answer = [file, total, handler = std::move(handler)](std::expected<void, ReplyError> result)
		{
			ZoneScopedN("writeFile_cb");
			reportTransfer(FileTransfer{.file = file, .sent = 0, .total = total, .finished = true});
			handler(std::move(result));
		};
		ask(std::move(answer),
			[file = std::move(file), contents = std::move(contents), total](Link& link, Handler<void> answer) mutable
			{
				reportTransfer(FileTransfer{.file = file, .sent = 0, .total = total, .finished = false});
				// Told whenever another whole percent has been sent, which is all the UI shows
				Progress progress = [file, percent = 0](size_t sent, size_t total) mutable
				{
					const int now = total == 0 ? 100 : static_cast<int>(sent * 100 / total);
					if (now != percent)
					{
						percent = now;
						reportTransfer(FileTransfer{.file = file, .sent = sent, .total = total, .finished = false});
					}
				};
				link.writeFile(file, std::move(contents), std::move(progress), std::move(answer));
			});
	}

	void Connection::deleteFile(std::string file, Handler<void> handler)
	{
		ZoneScoped;
		if (!handler)
		{
			handler = logFailure(fmt::format("Deleting {:s}", file));
		}
		ask(std::move(handler),
			[file = std::move(file)](Link& link, Handler<void> answer) { link.deleteFile(file, std::move(answer)); });
	}

	void Connection::connectThrough(std::unique_ptr<SerialPort> port)
	{
		ZoneScoped;
		// A task is copied, so the port is held where a copy shares it
		auto holder = std::make_shared<std::unique_ptr<SerialPort>>(std::move(port));
		post(
			[this, holder]
			{
				ZoneScopedN("connectThrough_cb");
				disconnect();
				m_retryAt.reset();
				beginConnecting();
				connectSerial(std::move(*holder));
			});
	}

	void Connection::after(std::chrono::milliseconds delay, std::function<void()> task)
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_queueMutex);
			m_timers.emplace(Clock::now() + delay, std::move(task));
		}
		m_queueChanged.notify_one();
	}

	void Connection::setManual(bool manual)
	{
		ZoneScoped;
		if (manual && m_running)
		{
			m_running = false;
			m_queueChanged.notify_all();
			if (m_thread.joinable())
			{
				m_thread.join();
			}
		}
		m_manual = manual;
	}

	void Connection::process()
	{
		ZoneScoped;
		runQueued();
		step(Clock::now());
		runQueued();
		m_linkWaiting = m_link && m_link->waiting();
	}

	void Connection::pollNow()
	{
		ZoneScoped;
		post(
			[this]
			{
				ZoneScopedN("pollNow_cb");
				if (m_sync)
				{
					m_sync->pollNow();
				}
			});
	}

	bool Connection::busy() const
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_queueMutex);
			if (!m_queue.empty())
			{
				return true;
			}
		}
		return m_linkWaiting.load();
	}

	void Connection::receiveUnasked(OM::Node& reply)
	{
		ZoneScoped;
		if (auto patch = OM::Patch::fromReply(reply))
		{
			if (m_sync)
			{
				m_sync->receiveUnasked(*patch);
			}
			else
			{
				OM::Tree::get().apply(*patch);
			}
		}
		else if (reply["resp"].exists())
		{
			showResponse(reply["resp"].text());
		}
		else if (reply["message"].exists())
		{
			LOG_INFO("Message: {:s}", reply["message"].text());
		}
		else if (reply["controlCommand"].exists())
		{
			controlCommand(reply["controlCommand"].text());
		}
		else
		{
			LOG_DBG("A reply nothing is waiting for: {:s}", reply.toJson());
		}
	}

	void Connection::post(std::function<void()> task)
	{
		ZoneScoped;
		{
			std::lock_guard lock(m_queueMutex);
			m_queue.push_back(std::move(task));
		}
		m_queueChanged.notify_one();
	}

	void Connection::run()
	{
		ZoneScoped;
		tracy::SetThreadName("Connection");
		while (m_running)
		{
			const Clock::time_point wake = nextWake();
			{
				std::unique_lock lock(m_queueMutex);
				m_queueChanged.wait_until(lock, wake, [this] { return !m_queue.empty() || !m_running; });
			}
			runQueued();
			step(Clock::now());
			m_linkWaiting = m_link && m_link->waiting();
		}
	}

	Connection::Clock::time_point Connection::nextWake() const
	{
		ZoneScoped;
		const Clock::time_point now = Clock::now();
		Clock::time_point wake = now + longestSleep;
		const auto noLaterThan = [&wake](Clock::time_point time) { wake = std::min(wake, time); };
		if (m_retryAt)
		{
			noLaterThan(*m_retryAt);
		}
		if (m_state == State::connecting)
		{
			noLaterThan(m_connectStarted + PRINTER_REQUEST_TIMEOUT);
		}
		if (m_link)
		{
			noLaterThan(m_link->lastHeard() + PRINTER_REQUEST_TIMEOUT);
			if (const auto due = m_link->nextDue())
			{
				noLaterThan(*due);
			}
		}
		if (m_sync)
		{
			noLaterThan(m_sync->nextPoll());
		}
		{
			std::lock_guard lock(m_queueMutex);
			if (!m_timers.empty())
			{
				noLaterThan(m_timers.begin()->first);
			}
		}
		return std::max(wake, now);
	}

	void Connection::runQueued()
	{
		ZoneScoped;
		while (true)
		{
			std::deque<std::function<void()>> tasks;
			{
				std::lock_guard lock(m_queueMutex);
				tasks.swap(m_queue);
				const auto due = m_timers.upper_bound(Clock::now());
				for (auto it = m_timers.begin(); it != due; ++it)
				{
					tasks.push_back(std::move(it->second));
				}
				m_timers.erase(m_timers.begin(), due);
			}
			TracyPlot("Connection tasks queued", static_cast<int64_t>(tasks.size()));
			if (tasks.empty())
			{
				return;
			}
			for (auto& task : tasks)
			{
				ZoneScopedN("Connection task");
				task();
			}
		}
	}

	void Connection::step(Clock::time_point now)
	{
		ZoneScoped;
		switch (m_state.load())
		{
		case State::disconnected:
			if (m_retryAt && now >= *m_retryAt)
			{
				m_retryAt.reset();
				connect();
			}
			return;
		case State::connecting:
			if (now - m_connectStarted > PRINTER_REQUEST_TIMEOUT)
			{
				lost("the printer didn't answer");
				return;
			}
			break;
		case State::connected:
			if (now - m_link->lastHeard() > PRINTER_REQUEST_TIMEOUT)
			{
				lost("the printer stopped answering");
				return;
			}
			break;
		}
		if (!m_link)
		{
			return; // waiting for rr_connect
		}
		if (m_link->failed())
		{
			lost("the link to the printer failed");
			return;
		}
		m_link->tick(now);
		if (m_sync)
		{
			m_sync->tick(now);
		}
	}

	void Connection::beginConnecting()
	{
		ZoneScoped;
		m_generation++;
		m_state = State::connecting;
		m_connectStarted = Clock::now();
		OM::SetStatus(OM::PrinterStatus::connecting);
	}

	void Connection::connect()
	{
		ZoneScoped;
		beginConnecting();
		LOG_INFO("Connecting by {:s}", nameof::nameof_enum(type()));

		switch (type())
		{
		case CommunicationType::usb:
			connectSerial(std::make_unique<UsbPort>(StorageHelper::getData(ID_ENABLE_SECOND_USB_CHANNEL)));
			break;
		case CommunicationType::uart:
			connectSerial(std::make_unique<UartPort>(DEFAULT_UART_PORT, StorageHelper::getData(ID_DUET_BAUD_RATE)));
			break;
		case CommunicationType::simulator:
#if DUET_SIMULATOR
			connectSerial(std::make_unique<SimulatorPort>());
#else
			lost("this build has no simulated Duet");
#endif
			break;
		case CommunicationType::network:
		{
			auto link = std::make_unique<HttpLink>(
				ipAddress(),
				password(),
				[this](std::function<void()> task) { post(std::move(task)); },
				[this](OM::Node& reply) { receiveUnasked(reply); });
			HttpLink& http = *link;
			m_link = std::move(link);
			http.open(
				[this, generation = m_generation](std::expected<HttpLink::Session, ReplyError> session)
				{
					ZoneScopedN("connect_cb");
					if (generation != m_generation)
					{
						return;
					}
					if (!session)
					{
						lost(fmt::format("rr_connect failed: {:s}", session.error().message));
						return;
					}
					OM::SetChannelIndex(0); // state.thisInput isn't sent by rr_model
					if (session->sbc)
					{
						connectDsf();
						return;
					}
					m_sync = std::make_unique<ModelSync>(*m_link);
					m_sync->setPollInterval(pollInterval());
					connected();
				});
			break;
		}
		}
	}

	void Connection::connectDsf()
	{
		ZoneScoped;
		LOG_INFO("The Duet has an SBC: talking to DSF");
		auto link = std::make_unique<DsfLink>(
			ipAddress(), password(), [this](std::function<void()> task) { post(std::move(task)); });
		DsfLink& dsf = *link;
		m_link = std::move(link); // the rr_* session's link goes
		dsf.open(
			[this, generation = m_generation](std::expected<void, ReplyError> opened)
			{
				ZoneScopedN("dsfOpen_cb");
				if (generation != m_generation)
				{
					return;
				}
				if (!opened)
				{
					lost(fmt::format("DSF couldn't be reached: {:s}", opened.error().message));
					return;
				}
				connected();
			});
	}

	void Connection::connectSerial(std::unique_ptr<SerialPort> port)
	{
		ZoneScoped;
		// Each channel's bytes are gathered into lines on the port's thread, and the lines handled on this one
		auto framers = std::make_shared<std::array<std::unique_ptr<LineFramer>, 2>>();
		for (size_t channel = 0; channel < framers->size(); channel++)
		{
			(*framers)[channel] = std::make_unique<LineFramer>(
				[this, generation = m_generation, channel](std::string_view line)
				{
					ZoneScopedN("lineFramed_cb");
					post([this, generation, channel, line = std::string(line)]
						 { lineReceived(generation, channel, line); });
				});
		}
		const bool opened = port->open(
			[framers](size_t channel, std::string_view bytes)
			{
				ZoneScopedN("portReceived_cb"); // on the port's thread
				ZoneValue(bytes.size());
				(*framers)[std::min(channel, framers->size() - 1)]->push(bytes);
			});
		if (!opened)
		{
			lost("the port couldn't be opened");
			return;
		}
		m_port = std::move(port);

		auto link = std::make_unique<SerialLink>(*m_port, [this](OM::Node& reply) { receiveUnasked(reply); });
		m_serial = link.get();
		m_link = std::move(link);
		for (const std::string& line : m_port->openingGcode())
		{
			m_link->sendGcode(line);
		}
		m_sync = std::make_unique<ModelSync>(*m_link);
		m_sync->setPollInterval(pollInterval());
		m_sync->pollNow(); // its answer says the printer is there
	}

	void Connection::connected()
	{
		ZoneScoped;
		LOG_INFO("Connected to the printer");
		m_state = State::connected;
		OM::Followers::refresh(); // the printer's status, in place of "connecting"
		Model::get().post<EventType::Connected>();
	}

	void Connection::disconnect()
	{
		ZoneScoped;
		const bool wasConnected = m_state == State::connected;
		const bool wasDisconnected = m_state == State::disconnected && !m_link && !m_port;
		m_generation++;
		m_serial = nullptr;
		if (!wasDisconnected)
		{
			// Before the link closes: what the file cache asked for is forgotten, not answered "closed" and asked again
			FileInfoCache::get().forget();
		}
		if (m_link)
		{
			m_link->close(); // whoever waits for an answer is told there is none
		}
		m_link.reset();
		m_sync.reset();
		if (m_port)
		{
			m_port->close();
			m_port.reset();
		}
		m_state = State::disconnected;
		if (wasDisconnected)
		{
			return;
		}
		LOG_INFO("Disconnected from the printer");
		OM::RemoveAll();
		if (wasConnected)
		{
			Model::get().post<EventType::Disconnected>();
		}
	}

	void Connection::lost(std::string_view reason)
	{
		ZoneScoped;
		ZoneText(reason.data(), reason.size());
		LOG_WARN("Connection lost: {:s}", reason);
		disconnect();
		m_retryAt = Clock::now() + retryDelay;
	}

	void Connection::lineReceived(uint64_t generation, size_t channel, std::string line)
	{
		ZoneScoped;
		if (generation != m_generation || m_serial == nullptr)
		{
			return; // from a port closed since
		}
		m_serial->receive(channel, line);
		// Connected once the printer has answered the live model request: any line could be noise (a wrong baud rate)
		if (m_state == State::connecting && m_sync && m_sync->hasLiveModel())
		{
			connected();
		}
	}
} // namespace Comm

/*
 * Connection.h
 *
 * The screen's connection to its printer, however it is made: USB, UART, the network (a standalone Duet, or one with
 * an SBC) or the simulated Duet. It is what the rest of the screen asks things of the printer through.
 *
 * Everything to do with the printer happens on the connection's own thread: connecting (and connecting again when
 * the printer stops answering), the Link that talks to it (Link.h), keeping the object model up to date (ModelSync.h,
 * or the link itself for DSF), and every answer. The methods here may be called from any thread: they queue the work
 * for the connection's thread and return at once, and an answer (a Handler) is given on the connection's thread.
 *
 *   disconnected ──connect──▶ connecting ──first reply──▶ connected
 *        ▲                        │                          │
 *        └──── wait, try again ◀──┴── no answer, port lost ◀─┘
 *
 * Tests run it by hand (setManual()): nothing happens until they call process().
 */

#pragma once

#include "Comm/Link.h"
#include "Comm/ModelSync.h"
#include "Comm/Ports/SerialPort.h"
#include "ObjectModel/Tree/Node.h"
#include "tracy/Tracy.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fmt/format.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace Comm
{
	class SerialLink;

	/** How the screen reaches the printer; the value is kept with the settings */
	enum class CommunicationType
	{
		usb,
		network,
		uart,
		simulator, // The simulated Duet (src/Simulator); only builds with DUET_SIMULATOR can choose it
	};

	/** The translation key of each CommunicationType's name */
	constexpr std::string_view communicationTypeNames[] = {
		"connection_method.usb",
		"connection_method.network",
		"connection_method.uart",
		"connection_method.simulator",
	};

	static_assert(magic_enum::enum_count<CommunicationType>() == std::size(communicationTypeNames),
				  "communicationTypeNames must have the same number of elements as CommunicationType");

	class Connection
	{
	  public:
		using Clock = std::chrono::steady_clock; // What times are measured with

		/** How long to wait before connecting again after a connection fails or is lost */
		static constexpr std::chrono::milliseconds retryDelay{1000};

		/** The connection */
		static Connection& get();

		/** Read the settings and start connecting, on the connection's thread (unless run by hand) */
		void start();

		/** Disconnect, and stop the connection's thread */
		void stop();

		// -- How it connects ------------------------------------------------------------------------------------------

		/** How the screen reaches the printer */
		CommunicationType type() const { return m_type.load(); }

		/** The translation key of how the screen reaches the printer */
		std::string_view typeName() const;

		/** Reach the printer by `type` from now on (kept with the settings), connecting again */
		void setType(CommunicationType type);

		/** How often the live object model is asked for */
		std::chrono::milliseconds pollInterval() const { return m_pollInterval.load(); }

		/** Ask for the live object model every `interval` (no less than MIN_PRINTER_POLL_INTERVAL), kept */
		void setPollInterval(std::chrono::milliseconds interval);

		/** The printer's network address */
		std::string ipAddress() const;

		/** Reach the printer at `address` over the network (kept), connecting again if that is how it is reached */
		void setIpAddress(std::string_view address);

		/** The printer's network password */
		std::string password() const;

		/** Use `password` over the network (kept), connecting again if that is how the printer is reached */
		void setPassword(std::string_view password);

		// -- The connection -------------------------------------------------------------------------------------------

		/** Whether the printer is connected and answering */
		bool isConnected() const { return m_state.load() == State::connected; }

		/** Whether the screen is connecting to the printer */
		bool isConnecting() const { return m_state.load() == State::connecting; }

		/** Disconnect and connect again */
		void reconnect();

		// -- Asking things of the printer (see Link.h); while not connected, answers say so --------------------------

		/** Send G-code: one or more lines; dropped while not connected */
		void sendGcode(std::string_view gcode);

		/** sendGcode() with formatted text */
		template <typename... Args>
		void sendGcodef(fmt::format_string<Args...> format, Args&&... args)
		{
			sendGcode(fmt::format(format, std::forward<Args>(args)...));
		}

		/** Stop the machine now and reset it, even while still connecting */
		void emergencyStop();

		/** One page of the folder `dir` from its item `first` on */
		void listFiles(std::string dir, size_t first, Handler<FileList> handler);

		/** What the firmware knows about the G-code file `file` */
		void fileInfo(std::string file, Handler<FileInfoReply> handler);

		/** The piece of a thumbnail in `file` starting at `offset` */
		void thumbnail(std::string file, uint32_t offset, Handler<FileChunk> handler);

		/** The whole of `file` */
		void readFile(std::string file, Handler<std::string> handler);

		/** Write `contents` to `file`, replacing it */
		void writeFile(std::string file, std::string contents, Handler<void> handler = nullptr);

		/** Delete `file` */
		void deleteFile(std::string file, Handler<void> handler = nullptr);

		// -- For tests
		// --------------------------------------------------------------------------------------------------

		/** Connect over `port`, which a test plays the printer on, as over USB; started at the next process() */
		void connectThrough(std::unique_ptr<SerialPort> port);

		/** Run only when told (process()), on the caller's thread; or (false) on a thread of its own */
		void setManual(bool manual);

		/** Run by hand: do everything queued and due now */
		void process();

		/** Ask for the live object model now, as the poll interval would */
		void pollNow();

		/** Whether anything is queued or waiting for the printer's answer */
		bool busy() const;

		/**
		 * Take a reply as one nobody asked for (as a status report is): object model replies are merged, responses
		 * shown. On the connection's thread (a test running it by hand).
		 */
		void receiveUnasked(OM::Node& reply);

	  private:
		/** Where the connection is */
		enum class State
		{
			disconnected,
			connecting,
			connected,
		};

		Connection();
		~Connection();
		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

		/** Queue `task` for the connection's thread */
		void post(std::function<void()> task);

		/** The connection's thread: run what is queued, and what is due, until stopped */
		void run();

		/** When the connection's thread next has something to do, if nothing is queued before then */
		Clock::time_point nextWake() const;

		/** Stop the connection's thread: wait for it, unless this is it (the program ending from it) */
		void stopThread();

		/** Run what is queued (the queue's lock not held) */
		void runQueued();

		/** Do what is due at `now`: connect, poll, give up requests, notice a printer that stopped answering */
		void step(Clock::time_point now);

		/** Note that connecting has started: a new generation, the printer's status "connecting" */
		void beginConnecting();

		/** Start connecting as the settings say */
		void connect();

		/** Open a serial port (USB, UART, the simulator, a test's) and start talking over it */
		void connectSerial(std::unique_ptr<SerialPort> port);

		/** rr_connect said the Duet has an SBC: talk to it through DSF's own API instead */
		void connectDsf();

		/** The printer answered: the connection is made */
		void connected();

		/** Close the link and port, forgetting the printer's state; tell everyone if it was connected */
		void disconnect();

		/** The connection failed or was lost, for `reason`: disconnect and try again after retryDelay */
		void lost(std::string_view reason);

		/** A whole line arrived on `channel` from the serial port opened as `generation` */
		void lineReceived(uint64_t generation, size_t channel, std::string line);

		/** Ask the link for something, or tell `handler` why not (not connected) */
		template <typename T, typename Ask>
		void ask(Handler<T> handler, Ask ask);

		std::atomic<CommunicationType> m_type{CommunicationType::usb}; // How the printer is reached
		std::atomic<std::chrono::milliseconds> m_pollInterval;		   // How often the live model is asked for
		mutable TracyLockable(std::mutex, m_settingsMutex);			   // Guards the settings below
		std::string m_ipAddress;									   // The printer's network address
		std::string m_password;										   // Its network password

		// Destroyed in the order opposite to this: the link before what it uses (the model sync and the port)
		std::atomic<State> m_state{State::disconnected}; // Where the connection is
		std::unique_ptr<SerialPort> m_port;				 // The serial port talked over, if the link is a SerialLink
		std::unique_ptr<ModelSync> m_sync;				 // Keeps the object model up to date, unless the link does
		std::unique_ptr<Link> m_link;					 // What talks to the printer, while connecting or connected
		SerialLink* m_serial = nullptr;					 // m_link, when it is a SerialLink (lines come from a port)
		std::atomic<bool> m_linkWaiting{false};			 // Whether the link has answers to come, for busy()
		uint64_t m_generation = 0;						 // Counts connections, so lines from an old port are dropped
		Clock::time_point m_connectStarted;				 // When connecting started
		std::optional<Clock::time_point> m_retryAt;		 // When to connect next, while disconnected

		mutable TracyLockable(std::mutex, m_queueMutex); // Guards m_queue
		std::condition_variable_any m_queueChanged;		 // Wakes the thread when something is queued
		std::deque<std::function<void()>> m_queue;		 // Tasks for the connection's thread
		bool m_manual = false;							 // Run by hand (tests)
		std::atomic<bool> m_running{false};				 // The thread carries on
		std::thread m_thread;							 // The connection's thread
	};

} // namespace Comm

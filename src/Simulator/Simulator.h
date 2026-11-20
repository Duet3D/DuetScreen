/*
 * Simulator.h
 *
 * The simulated Duet, as a connection: the screen's connection sends it numbered G-code lines as it would a Duet on
 * USB (CommunicationType::simulator, through Comm::SimulatorPort), and it answers as a Duet does, a line of JSON per
 * reply, numbered with the line it answers as RepRapFirmware 3.6.2 on does. Behind it is a
 * Machine (Machine.h), built from a Config the screen keeps with its settings and edits on Settings > Connection >
 * Simulated Duet.
 *
 * In the program it runs on a thread of its own: requests are answered after the reply delay, and time passes for
 * the machine as it does for the screen. Tests switch it to manual (setManual()), so nothing happens until they say:
 * process() answers what was sent, advance() lets time pass.
 *
 * It can also misbehave, to test how the screen copes: stop answering, be unplugged (connecting fails), answer slowly.
 *
 * Built only with the DUET_SIMULATOR build option (development builds and the unit tests).
 */

#pragma once

#include "Config.h"
#include "Machine.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace Sim
{
	/** How the simulated Duet behaves as a connection; not kept: every start is well behaved */
	struct Behaviour
	{
		int32_t replyDelayMs = 0; // Each request is answered this long after it was sent
		bool responding = true;	  // Answers requests; if not, the screen times out and reconnects
		bool pluggedIn = true;	  // Can be connected to; unplugged, it doesn't answer either
		float noise = 0.3f; // Noise on a heater's readings while it holds its temperature, °C (standard deviation)
	};

	/** The simulated Duet. See the file comment. */
	class Simulator
	{
	  public:
		/** Receives each reply: one JSON object, as text */
		using Receiver = std::function<void(std::string_view reply)>;

		/** The one simulated Duet */
		static Simulator& get();

		/** Connect, sending replies to `receiver` from now on. @return false while unplugged */
		bool connect(Receiver receiver);

		/** Disconnect: requests waiting are dropped and nothing more is sent */
		void disconnect();

		/** Whether it is connected */
		bool connected() const;

		/** Send it G-code: one or more lines, each answered in turn */
		void send(std::string_view gcode);

		/** What it is */
		Config config() const;

		/** Become `config` and keep it with the settings; what still applies carries on (temperatures, a job) */
		void setConfig(Config config);

		/** How it behaves */
		Behaviour behaviour() const;

		/** Behave as `behaviour` says */
		void setBehaviour(Behaviour behaviour);

		/** Force the status it reports, e.g. "halted"; nothing to report what it is doing */
		void forceStatus(std::optional<std::string> status);

		/** The status forced, if any */
		std::optional<std::string> forcedStatus() const;

		/** Show a message box of `mode` (0-7) with example content, as M291 would */
		void showMessageBox(int32_t mode);

		/** Start again: the configuration kept with the settings, a new machine, well behaved, disconnected */
		void reset();

		// For tests

		/** Run only when told (process(), advance()), on the caller's thread, or (false) on a thread of its own */
		void setManual(bool manual);

		/** Answer every request sent, and any they lead to (manual) */
		void process();

		/** Let `time` pass for the machine (manual) */
		void advance(std::chrono::milliseconds time);

		/** Act on the machine itself, with the simulator locked; e.g. to check where an axis is */
		template <typename F>
		auto withMachine(F&& f)
		{
			std::lock_guard lock(m_lock);
			return f(m_machine);
		}

	  private:
		Simulator();
		~Simulator();
		Simulator(const Simulator&) = delete;
		Simulator& operator=(const Simulator&) = delete;

		/** A line sent, waiting to be answered */
		struct Request
		{
			std::string line;						   // The line
			std::chrono::steady_clock::time_point due; // When to answer it
		};

		/** The thread's loop: time passes, requests are answered */
		void run();

		/** Answer the requests due by `now`; @return the replies, to send without the lock (the lock held) */
		std::vector<std::string> answer(std::chrono::steady_clock::time_point now);

		/** Send `replies` to the receiver (the lock not held) */
		void deliver(const std::vector<std::string>& replies);

		/** Keep the machine's configuration with the settings (the lock held) */
		void save() const;

		/** Start the thread, unless manual or already running (the lock held) */
		void start();

		/** Stop the thread and wait for it (the lock not held) */
		void stop();

		mutable std::mutex m_lock;			// Guards everything below but the thread's flag
		Machine m_machine;					// The machine behind it
		Behaviour m_behaviour;				// How it behaves
		std::deque<Request> m_requests;		// Lines waiting to be answered
		Receiver m_receiver;				// Where replies go; empty while disconnected
		bool m_manual = false;				// Driven by a test (process(), advance())
		std::thread m_thread;				// Runs it, unless manual
		std::atomic<bool> m_running{false}; // Tells the thread to carry on
	};
} // namespace Sim

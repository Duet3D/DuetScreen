/*
 * WifiScanner.h
 *
 * Finds the WiFi networks around the screen, every few seconds while asked to, and connects to them, on threads of
 * its own. Those threads never touch the UI: they post Model events (WifiNetworks when a scan finishes,
 * WifiConnectResult when a connection attempt ends), which presenters receive like any other. So joining them, when
 * the scanner is destroyed with its presenter on the UI thread, can't wait for the UI lock.
 */

#pragma once

#include "utils/NetworkHelper.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace UI
{
	class WifiScanner
	{
	  public:
		/** Start the scanning thread, paused */
		WifiScanner();

		/** Stop the threads and wait for them */
		~WifiScanner();

		WifiScanner(const WifiScanner&) = delete;
		WifiScanner& operator=(const WifiScanner&) = delete;

		/** Scan now and every few seconds (true), or stop scanning (false) */
		void setScanning(bool scanning);

		/** Scan as soon as possible, e.g. after connecting or forgetting a network */
		void scanNow();

		/** The networks the last scan found */
		std::vector<WiFiNetwork> networks() const;

		/** Connect to `ssid`, with `password` if it isn't known yet, unless a connection attempt is under way */
		void connect(std::string ssid, std::optional<std::string> password = std::nullopt);

	  private:
		/** The scanning thread: scan whenever asked to, and every few seconds while scanning */
		void scanLoop(std::stop_token stop);

		mutable std::mutex m_mutex;			   // Guards m_networks and the scan requests
		std::condition_variable_any m_wake;	   // Wakes the scanning thread
		std::vector<WiFiNetwork> m_networks;   // What the last scan found
		bool m_scanning = false;			   // Scanning every few seconds
		bool m_scanRequested = false;		   // A scan is wanted now
		std::atomic<bool> m_connecting{false}; // A connection attempt is under way
		std::jthread m_connectThread;		   // The last connection attempt
		std::jthread m_scanThread;			   // Scans; declared last, so it starts after the rest exists
	};
} // namespace UI

/*
 * NetworkHelper.cpp
 *
 *  Created on: 2025-01-30
 *      Author: Andy Everitt
 */

#include "NetworkHelper.h"
#include "Debug.h"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ifaddrs.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <thread>
#include <vector>

#if T113
#  include <wpa_ctrl.h>
#endif

namespace NetworkHelper
{
#define CTRL_PATH "/var/run/wpa_supplicant"
#define WPA_SUPPLICANT_CONF "/etc/wpa_supplicant.conf"

	static constexpr int s_timeout_ms = 10000;
	/// Maximum time to wait for the wireless interface to appear after enabling.
	static constexpr auto s_enableTimeout = std::chrono::seconds(15);
	/// Polling interval while waiting for interface / wpa_supplicant readiness.
	static constexpr auto s_enablePollInterval = std::chrono::milliseconds(500);
	/// Time to wait for wpa_supplicant to report a connection result after SELECT_NETWORK.
	static constexpr auto s_connectTimeout = std::chrono::seconds(15);
	/// Polling interval while waiting for a connection result.
	static constexpr auto s_connectPollInterval = std::chrono::milliseconds(500);

	static TracyLockable(std::mutex, s_mutex);

	[[maybe_unused]] static struct wpa_ctrl* s_ctrl_conn = nullptr;
	[[maybe_unused]] static struct wpa_ctrl* s_monitor_conn = nullptr;
	static std::vector<WiFiNetwork> s_networks;
	/// Cached wireless interface name (e.g. "wlan0", "wlan1").
	static std::string s_interfaceName;
	/// Background thread used for async enable.
	static std::thread s_enableThread;
	/// Guards s_enabling and s_enableCallbacks.
	static TracyLockable(std::mutex, s_enableMutex);
	/// Set while an enable operation is in progress, to prevent overlapping enable operations.
	static bool s_enabling = false;
	/// Callbacks to invoke when the in-progress enable operation completes.
	static std::vector<std::function<void(bool)>> s_enableCallbacks;

	/// Discover the wireless interface name from /sys/class/net.
	/// Returns empty string if no wireless interface is found.
	[[maybe_unused]] static std::string discoverWirelessInterface()
	{
		ZoneScoped;
#if T113
		namespace fs = std::filesystem;
		const fs::path netClass{"/sys/class/net"};
		if (!fs::exists(netClass))
			return {};

		for (const auto& entry : fs::directory_iterator(netClass))
		{
			const auto name = entry.path().filename().string();
			// Skip loopback
			if (name == "lo")
				continue;
			// A wireless interface has a "wireless" or "phy80211" subdirectory
			if (fs::exists(entry.path() / "wireless") || fs::exists(entry.path() / "phy80211"))
			{
				LOG_INFO("Discovered wireless interface: {:s}", name);
				return name;
			}
		}
		LOG_WARN("No wireless interface found in /sys/class/net");
		return {};
#else
		return "wlan0";
#endif
	}

	/// Get the cached interface name, discovering it if needed.
	static const std::string& getInterfaceName()
	{
		if (s_interfaceName.empty())
		{
			s_interfaceName = discoverWirelessInterface();
		}
		return s_interfaceName;
	}

	/// Build the wpa_supplicant control socket path for the current interface.
	[[maybe_unused]] static std::string getCtrlPath()
	{
		return std::string(CTRL_PATH "/") + getInterfaceName();
	}

	/// Discover an ethernet interface from /sys/class/net.
	/// Returns empty string if no likely ethernet interface is found.
	static std::string discoverEthernetInterface()
	{
		ZoneScoped;
#if T113
		namespace fs = std::filesystem;
		const fs::path netClass{"/sys/class/net"};
		if (!fs::exists(netClass))
			return {};

		std::string fallbackInterface;
		for (const auto& entry : fs::directory_iterator(netClass))
		{
			const auto name = entry.path().filename().string();
			if (name == "lo")
				continue;

			const bool isWireless = fs::exists(entry.path() / "wireless") || fs::exists(entry.path() / "phy80211");
			if (isWireless)
				continue;

			if (name.rfind("eth", 0) == 0 || name.rfind("en", 0) == 0)
			{
				LOG_INFO("Discovered ethernet interface: {:s}", name);
				return name;
			}

			if (fallbackInterface.empty())
				fallbackInterface = name;
		}

		if (!fallbackInterface.empty())
		{
			LOG_INFO("Using fallback ethernet interface: {:s}", fallbackInterface);
			return fallbackInterface;
		}

		LOG_WARN("No ethernet interface found in /sys/class/net");
		return {};
#else
		return "eth0";
#endif
	}

	/// Get the IPv4 address currently assigned to a specific network interface.
	static std::string getIpv4AddressForInterface(std::string_view interfaceName)
	{
		if (interfaceName.empty())
			return {};

		struct ifaddrs* ifaddr = nullptr;
		if (getifaddrs(&ifaddr) == -1)
		{
			LOG_ERROR("getifaddrs failed while checking {:s}", interfaceName);
			return {};
		}

		std::string ipAddress;
		for (struct ifaddrs* p = ifaddr; p != nullptr; p = p->ifa_next)
		{
			if (p->ifa_addr == nullptr || p->ifa_addr->sa_family != AF_INET)
				continue;

			if (interfaceName != p->ifa_name)
				continue;

			char buffer[INET_ADDRSTRLEN] = {0};
			auto* addrIn = reinterpret_cast<struct sockaddr_in*>(p->ifa_addr);
			if (inet_ntop(AF_INET, &addrIn->sin_addr, buffer, sizeof(buffer)) != nullptr)
			{
				ipAddress = buffer;
				break;
			}
		}

		freeifaddrs(ifaddr);
		return ipAddress;
	}

	[[maybe_unused]] static bool initWPAControl()
	{
		ZoneScoped;
		LOG_DBG("Initializing wpa_supplicant control interface");
#if T113
		if (s_ctrl_conn != nullptr)
			return true;

		std::string ctrl_path = getCtrlPath();
		s_ctrl_conn = wpa_ctrl_open(ctrl_path.c_str());
		if (s_ctrl_conn == nullptr)
		{
			LOG_ERROR("Failed to connect to wpa_supplicant at {:s}", ctrl_path);
			return false;
		}

		s_monitor_conn = wpa_ctrl_open(ctrl_path.c_str());
		if (s_monitor_conn == nullptr)
		{
			wpa_ctrl_close(s_ctrl_conn);
			s_ctrl_conn = nullptr;
			LOG_ERROR("Failed to open monitor connection");
			return false;
		}

		if (wpa_ctrl_attach(s_monitor_conn) != 0)
		{
			wpa_ctrl_close(s_monitor_conn);
			wpa_ctrl_close(s_ctrl_conn);
			s_monitor_conn = nullptr;
			s_ctrl_conn = nullptr;
			LOG_ERROR("Failed to attach to wpa_supplicant");
			return false;
		}

		return true;
#else
		return false;
#endif
	}

	[[maybe_unused]] static void closeWPAControl()
	{
		ZoneScoped;
		LOG_DBG("Closing wpa_supplicant control interface");
		std::lock_guard<LockableBase(std::mutex)> lock(s_mutex);
#if T113
		if (s_monitor_conn != nullptr)
		{
			wpa_ctrl_detach(s_monitor_conn);
			wpa_ctrl_close(s_monitor_conn);
			s_monitor_conn = nullptr;
		}
		if (s_ctrl_conn != nullptr)
		{
			wpa_ctrl_close(s_ctrl_conn);
			s_ctrl_conn = nullptr;
		}
#endif
	}

	static std::string sendCommand(const std::string& cmd)
	{
		ZoneScoped;
		std::lock_guard<LockableBase(std::mutex)> lock(s_mutex);
#if T113
		if (!initWPAControl())
		{
			LOG_ERROR("Failed to initialize wpa_supplicant control interface, could not send command: {:s}", cmd);
			return "";
		}

		LOG_DBG("Sending command: {:s}", cmd);
		char buf[4096];
		size_t len = sizeof(buf) - 1;

		int ret = wpa_ctrl_request(s_ctrl_conn, cmd.c_str(), cmd.length(), buf, &len, nullptr);
		if (ret < 0)
		{
			LOG_ERROR("Failed to send command: {:s}", cmd);
			return "";
		}

		buf[len] = '\0';
		return std::string(buf);
#else
		LOG_DBG("Simulating sending command: {:s}", cmd);
		return "";
#endif
	}

	/// Query STATUS and return the value of a given key, or empty string.
	static std::string getStatusField(const std::string& status, std::string_view key)
	{
		auto searchKey = std::string(key) + "=";
		size_t pos = status.find(searchKey);
		if (pos == std::string::npos)
			return {};
		size_t valStart = pos + searchKey.size();
		size_t end = status.find('\n', valStart);
		if (end == std::string::npos)
			return status.substr(valStart);
		return status.substr(valStart, end - valStart);
	}

	/// Internal blocking implementation of the enable logic.
	/// Called from a background thread when enabling, or inline when disabling.
	[[maybe_unused]] static bool enableBlocking(bool doEnable)
	{
		ZoneScoped;
		LOG_INFO("{:s} WiFi", doEnable ? "Enabling" : "Disabling");
#if T113
		// Invalidate cached interface name so we re-discover after link changes
		s_interfaceName.clear();
		// Close any stale control connection so it reconnects with new interface
		closeWPAControl();

		if (!doEnable)
		{
			// When disabling, try to find the interface first
			const auto& iface = getInterfaceName();
			if (!iface.empty())
			{
				std::string cmd = fmt::format("ip link set {:s} down", iface);
				int32_t errorCode = system(cmd.c_str());
				if (errorCode != 0)
				{
					LOG_ERROR("Failed to disable WiFi (ip link down), code={:d}", errorCode);
					return false;
				}
			}
			return true;
		}

		// --- Enabling: bring the link up and wait for the interface to appear ---
		const auto deadline = std::chrono::steady_clock::now() + s_enableTimeout;

		// Try bringing up any known interface first; if none found yet, wait for it
		for (;;)
		{
			ZoneScopedN("Polling for wireless interface");
			const auto& iface = getInterfaceName();
			if (!iface.empty())
			{
				std::string cmd = fmt::format("ip link set {:s} up", iface);
				int32_t errorCode = system(cmd.c_str());
				if (errorCode != 0)
				{
					LOG_WARN("ip link set {:s} up failed (code={:d}), will retry", iface, errorCode);
				}
				else
				{
					LOG_INFO("WiFi interface {:s} brought up", iface);
					break;
				}
			}

			if (std::chrono::steady_clock::now() >= deadline)
			{
				LOG_ERROR("Timed out waiting for wireless interface to appear");
				return false;
			}

			LOG_DBG("Wireless interface not yet available, retrying...");
			// Clear cached name so next iteration re-scans
			s_interfaceName.clear();
			std::this_thread::sleep_for(s_enablePollInterval);
		}

		// --- Wait for wpa_supplicant control interface to be ready ---
		const std::string ctrlSocketPath = getCtrlPath();
		bool wpaReady = false;
		while (std::chrono::steady_clock::now() < deadline)
		{
			if (std::filesystem::exists(ctrlSocketPath))
			{
				wpaReady = true;
				break;
			}

			// Start wpa_supplicant if not running
			LOG_DBG("wpa_supplicant control socket not found, starting wpa_supplicant");
			const auto& iface = getInterfaceName();
			std::string cmd = fmt::format("wpa_supplicant -B -i {:s} -c " WPA_SUPPLICANT_CONF, iface);
			int32_t errorCode = system(cmd.c_str());
			if (errorCode != 0)
			{
				LOG_WARN("wpa_supplicant start returned code={:d}, will retry", errorCode);
			}

			std::this_thread::sleep_for(s_enablePollInterval);
		}

		if (!wpaReady && !std::filesystem::exists(ctrlSocketPath))
		{
			LOG_ERROR("wpa_supplicant control socket {:s} did not appear within timeout", ctrlSocketPath);
			return false;
		}

		// Give wpa_supplicant a moment to fully initialise before accepting commands
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		LOG_INFO("WiFi enabled successfully, interface {:s}", getInterfaceName());
		return true;
#else
		(void)doEnable;
		return true;
#endif
	}

	void enable(bool doEnable, std::function<void(bool)> callback)
	{
		ZoneScoped;

		// Disabling is fast — run inline
		if (!doEnable)
		{
			bool ok = enableBlocking(false);
			if (callback)
				callback(ok);
			return;
		}

		std::lock_guard<LockableBase(std::mutex)> lock(s_enableMutex);
		if (callback)
			s_enableCallbacks.push_back(std::move(callback));

		// Prevent overlapping enable attempts, the callback is invoked when the in-progress attempt completes
		if (s_enabling)
		{
			LOG_DBG("WiFi enable already in progress, waiting for it to complete");
			return;
		}
		s_enabling = true;

		// Clean up any previous enable thread (can't join ourselves if called from a completion callback)
		if (s_enableThread.joinable())
		{
			if (s_enableThread.get_id() == std::this_thread::get_id())
				s_enableThread.detach();
			else
				s_enableThread.join();
		}

		// Launch the blocking work on a background thread
		s_enableThread = std::thread(
			[]()
			{
				tracy::SetThreadName("WifiEnableThread");
				bool ok = enableBlocking(true);

				std::vector<std::function<void(bool)>> callbacks;
				{
					std::lock_guard<LockableBase(std::mutex)> lock(s_enableMutex);
					callbacks.swap(s_enableCallbacks);
					s_enabling = false;
				}
				for (auto& cb : callbacks)
					cb(ok);
			});
	}

	bool isEnabled()
	{
		ZoneScoped;
		std::string output = sendCommand("STATUS");
		LOG_DBG("WiFi status: {:s}", output);
		return output.find("wpa_state=COMPLETED") != std::string::npos;
	}

	void reconfigure()
	{
		ZoneScoped;
		LOG_INFO("Reconfiguring wpa_supplicant");
		sendCommand("RECONFIGURE");
	}

	std::string getIpAddress()
	{
		ZoneScoped;
#if USE_FIXED_TEST_STRINGS
		return "192.168.1.100";
#endif

		std::string output = sendCommand("STATUS");
		if (auto ipAddress = getStatusField(output, "ip_address"); !ipAddress.empty())
		{
			return ipAddress;
		}

		const auto ethernetInterface = discoverEthernetInterface();
		if (ethernetInterface.empty())
			return {};

		return getIpv4AddressForInterface(ethernetInterface);
	}

	std::vector<WiFiNetwork> getKnownWiFiNetworks()
	{
		ZoneScoped;
		LOG_INFO("Getting known WiFi networks");
		std::vector<WiFiNetwork> networks;

		std::string output = sendCommand("LIST_NETWORKS");
		std::istringstream stream(output);
		std::string line;
		bool header = true;

		while (std::getline(stream, line))
		{
			if (header)
			{
				header = false;
				continue;
			}

			// LIST_NETWORKS columns are tab-separated: id\tssid\tbssid\tflags
			std::string idStr, ssid, bssid, flags;
			{
				// Split by tabs to preserve spaces inside SSID
				std::array<std::string, 4> cols;
				std::string::size_type start = 0;

				size_t count = 0;
				for (size_t i = 0; i < cols.size(); ++i)
				{
					++count;
					auto pos = line.find('\t', start);
					if (pos == std::string::npos)
					{
						cols[i] = (line.substr(start));
						break;
					}
					cols[i] = (line.substr(start, pos - start));
					start = pos + 1;
				}

				if (count < 4)
				{
					LOG_WARN("Unexpected LIST_NETWORKS format");
					continue;
				}

				idStr = std::move(cols[0]);
				ssid = std::move(cols[1]);
				bssid = std::move(cols[2]);
				flags = std::move(cols[3]);
			}

			WiFiNetwork network;
			// Remove quotes if present (wpa_supplicant may quote SSID)
			if (ssid.size() >= 2 && ssid.front() == '"' && ssid.back() == '"')
			{
				ssid = ssid.substr(1, ssid.size() - 2);
			}
			network.ssid = ssid;
			// Guard id parsing
			try
			{
				network.id = std::stoi(idStr);
			}
			catch (...)
			{
				continue;
			}
			network.connected = (flags.find("[CURRENT]") != std::string::npos);
			networks.push_back(network);
			LOG_INFO("Found known network: \"{:s}\", id: {:d}{:s}",
					 network.ssid.c_str(),
					 network.id,
					 network.connected ? " [CURRENT]" : "");
		}
		return networks;
	}

	std::vector<WiFiNetwork> scanWiFiNetworks()
	{
		ZoneScoped;
		LOG_INFO("Scanning for WiFi networks");
#if SIMULATION
		std::vector<WiFiNetwork> networks = {{.ssid = "Network 1", .signal_level = 100, .id = 1, .connected = true},
											 {.ssid = "Network 2", .signal_level = 75, .id = 2, .connected = false},
											 {.ssid = "Network 3", .signal_level = 50, .id = 3, .connected = false},
											 {.ssid = "Network 4", .signal_level = 25, .id = -1, .connected = false}};
		return networks;
#else
		std::vector<WiFiNetwork> networks;
		std::vector<WiFiNetwork> knownNetworks = getKnownWiFiNetworks();

		sendCommand("SCAN");
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 milliseconds for scan to complete

		std::string output = sendCommand("SCAN_RESULTS");
		std::istringstream stream(output);
		std::string line;
		bool header = true;

		while (std::getline(stream, line))
		{
			if (header)
			{
				header = false;
				continue;
			}

			std::istringstream iss(line);
			std::string bssid, freq, signal, flags, ssid;
			if (iss >> bssid >> freq >> signal >> flags)
			{
				// Rest of the line is SSID
				std::getline(iss, ssid);
				while (!ssid.empty() && (ssid[0] == ' ' || ssid[0] == '\t'))
				{
					ssid.erase(0, 1);
				}

				if (!ssid.empty())
				{
					WiFiNetwork network;
					network.ssid = ssid;
					network.signal_level = std::stoi(signal);
					network.id = -1;

					// Check if this is a known network
					for (const WiFiNetwork& known : knownNetworks)
					{
						if (network.ssid == known.ssid)
						{
							network.id = known.id;
							network.connected = known.connected;
							break;
						}
					}

					networks.push_back(network);
					LOG_INFO("Found network: \"{:s}\", signal: {:d} dBm, id: {:d}{:s}",
							 network.ssid.c_str(),
							 network.signal_level,
							 network.id,
							 network.connected	 ? " [CURRENT]"
							 : network.isKnown() ? " [KNOWN]"
												 : "");
				}
			}
		}

		// Sort networks by signal strength and connection status
		std::sort(networks.begin(),
				  networks.end(),
				  [](const WiFiNetwork& a, const WiFiNetwork& b)
				  {
					  if (a.connected != b.connected)
						  return a.connected;
					  if (a.id > b.id)
						  return true;
					  return a.signal_level > b.signal_level;
				  });

		return networks;
#endif
	}

	bool isNetworkKnown(std::string_view ssid)
	{
		ZoneScoped;
		std::vector<WiFiNetwork> networks = getKnownWiFiNetworks();
		return std::any_of(
			networks.begin(), networks.end(), [&ssid](const WiFiNetwork& network) { return network.ssid == ssid; });
	}

	/// Poll wpa_supplicant STATUS until association completes, fails, or times out.
	static ConnectResult waitForConnectionResult()
	{
		ZoneScoped;
		const auto deadline = std::chrono::steady_clock::now() + s_connectTimeout;

		while (std::chrono::steady_clock::now() < deadline)
		{
			std::this_thread::sleep_for(s_connectPollInterval);

			std::string status = sendCommand("STATUS");
			std::string wpaState = getStatusField(status, "wpa_state");

			LOG_DBG("Connection poll: wpa_state={:s}", wpaState);

			if (wpaState == "COMPLETED")
			{
				LOG_INFO("WiFi connection completed successfully");
				return ConnectResult::Success;
			}

			// DISCONNECTED after an attempt usually means auth failure.
			// wpa_supplicant also sets wpa_state=DISCONNECTED when the 4-way
			// handshake fails (wrong PSK).
			if (wpaState == "DISCONNECTED" || wpaState == "INACTIVE")
			{
				// Distinguish auth failure from other disconnects by checking
				// if there was a recent CTRL-EVENT-SSID-TEMP-DISABLED or
				// CTRL-EVENT-DISCONNECTED reason=WRONG_KEY in the status.
				// A simpler heuristic: if we just asked to connect and we're
				// already DISCONNECTED, it's almost certainly an auth failure
				// (the scan found the AP, but the handshake failed).
				LOG_WARN("WiFi connection failed (wpa_state={:s}), likely authentication failure", wpaState);
				return ConnectResult::AuthFailure;
			}

			// SCANNING, ASSOCIATING, 4WAY_HANDSHAKE etc. are transient — keep waiting
		}

		LOG_WARN("WiFi connection timed out");
		return ConnectResult::Timeout;
	}

	ConnectResult connect(std::string_view ssid)
	{
		ZoneScoped;
		LOG_INFO("Connecting to WiFi network \"{:s}\"", ssid);
		std::vector<WiFiNetwork> networks = getKnownWiFiNetworks();
		for (const WiFiNetwork& network : networks)
		{
			if (network.ssid == ssid)
			{
				std::string cmd = fmt::format("SELECT_NETWORK {}", network.id);
				sendCommand(cmd);
				return waitForConnectionResult();
			}
		}
		LOG_ERROR("Network \"{:s}\" not found in known networks", ssid);
		return ConnectResult::NetworkNotFound;
	}

	ConnectResult connect(std::string_view ssid, std::string_view password)
	{
		ZoneScoped;
		LOG_INFO("Connecting to WiFi network \"{:s}\"", ssid);
#if T113
		if (isNetworkKnown(ssid))
		{
			// Network is already known, remove existing entry first
			forgetNetwork(ssid);
		}

		std::string cmd = "ADD_NETWORK";
		std::string output = sendCommand(cmd);
		int networkId = -1;
		try
		{
			networkId = std::stoi(output);
		}
		catch (...)
		{
			LOG_ERROR("Failed to parse network id from ADD_NETWORK response: {:s}", output);
			return ConnectResult::Error;
		}

		cmd = fmt::format("SET_NETWORK {:d} ssid \"{:s}\"", networkId, ssid);
		sendCommand(cmd);

		cmd = fmt::format("SET_NETWORK {:d} psk \"{:s}\"", networkId, password);
		sendCommand(cmd);

		cmd = fmt::format("ENABLE_NETWORK {}", networkId);
		sendCommand(cmd);

		cmd = fmt::format("SELECT_NETWORK {}", networkId);
		sendCommand(cmd);

		ConnectResult result = waitForConnectionResult();

		if (result == ConnectResult::Success)
		{
			sendCommand("SAVE_CONFIG");
		}
		else
		{
			// Remove the network entry if connection failed so it doesn't persist
			// with wrong credentials
			cmd = fmt::format("REMOVE_NETWORK {}", networkId);
			sendCommand(cmd);
		}

		return result;
#else
		UNUSED(password);
		return ConnectResult::Success;
#endif
	}

	void disconnect()
	{
		ZoneScoped;
		LOG_INFO("Disconnecting from WiFi network");
		sendCommand("DISCONNECT");
	}

	void reconnect()
	{
		ZoneScoped;
		LOG_INFO("Reconnecting to WiFi network");
		sendCommand("RECONNECT");
	}

	void forgetNetwork(std::string_view ssid)
	{
		ZoneScoped;
		LOG_INFO("Forgetting network \"{:s}\"", ssid);
		std::vector<WiFiNetwork> networks = getKnownWiFiNetworks();
		for (const WiFiNetwork& network : networks)
		{
			if (network.ssid == ssid)
			{
				std::string cmd = "REMOVE_NETWORK " + std::to_string(network.id);
				sendCommand(cmd);
				sendCommand("SAVE_CONFIG");
				return;
			}
		}
		LOG_ERROR("Network \"{:s}\" not found in known networks", ssid);
	}
} // namespace NetworkHelper
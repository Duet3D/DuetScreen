/*
 * NetworkHelper.cpp
 *
 *  Created on: 2025-01-30
 *      Author: Andy Everitt
 */

#include "NetworkHelper.h"
#include "Debug.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if T113
#  include <wpa_ctrl.h>
#endif

namespace NetworkHelper
{
	static const std::string INTERFACE = "wlan0";
	static constexpr const char* s_wpa_supplicant = "/etc/wpa_supplicant.conf";
	static constexpr const char* s_ctrl_path = "/var/run/wpa_supplicant";
	static constexpr int s_timeout_ms = 10000;

	static struct wpa_ctrl* s_ctrl_conn = nullptr;
	static struct wpa_ctrl* s_monitor_conn = nullptr;
	static std::vector<WiFiNetwork> s_networks;

#if T113
	static bool initWPAControl()
	{
		if (s_ctrl_conn != nullptr)
			return true;

		std::string ctrl_path = std::string(s_ctrl_path) + "/" + INTERFACE;
		s_ctrl_conn = wpa_ctrl_open(ctrl_path.c_str());
		if (s_ctrl_conn == nullptr)
		{
			LOG_ERROR("Failed to connect to wpa_supplicant");
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
	}

	static void closeWPAControl()
	{
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
	}

	static std::string sendCommand(const std::string& cmd)
	{
		if (!initWPAControl())
			return "";

		char buf[4096];
		size_t len = sizeof(buf) - 1;

		int ret = wpa_ctrl_request(s_ctrl_conn, cmd.c_str(), cmd.length(), buf, &len, nullptr);
		if (ret < 0)
		{
			LOG_ERROR("Failed to send command: {:s}", cmd.c_str());
			return "";
		}

		buf[len] = '\0';
		return std::string(buf);
	}
#else
	static bool initWPAControl()
	{
		return false;
	}
	static void closeWPAControl() {}
	static std::string sendCommand(const std::string&)
	{
		return "";
	}
#endif

	void enable(bool enable)
	{
		LOG_INFO("{:s} WiFi", enable ? "Enabling" : "Disabling");
		std::string cmd = "ip link set " + INTERFACE + (enable ? " up" : " down");
		int32_t errorCode = system(cmd.c_str());
		if (errorCode != 0)
		{
			LOG_ERROR("Failed to {:s} WiFi, code={:d}", enable ? "enable" : "disable", errorCode);
		}
	}

	bool isEnabled()
	{
		std::string output = sendCommand("STATUS");
		LOG_DBG("WiFi status: {:s}", output);
		return output.find("wpa_state=COMPLETED") != std::string::npos;
	}

	void reconfigure()
	{
		LOG_INFO("Reconfiguring wpa_supplicant");
		sendCommand("RECONFIGURE");
	}

	std::string getIpAddress()
	{
		std::string result;
		std::string output = sendCommand("STATUS");

		size_t pos = output.find("ip_address=");
		if (pos != std::string::npos)
		{
			size_t end = output.find('\n', pos);
			if (end != std::string::npos)
			{
				result = output.substr(pos + 11, end - (pos + 11));
			}
		}
		return result;
	}

	std::vector<WiFiNetwork> getKnownWiFiNetworks()
	{
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

			std::istringstream iss(line);
			std::string idStr, ssid, bssid, flags;
			if (iss >> idStr >> ssid >> bssid >> flags)
			{
				WiFiNetwork network;
				// Remove quotes if present
				if (ssid.size() >= 2 && ssid.front() == '"' && ssid.back() == '"')
				{
					ssid = ssid.substr(1, ssid.size() - 2);
				}
				network.ssid = ssid;
				network.id = std::stoi(idStr);
				network.connected = (flags.find("[CURRENT]") != std::string::npos);
				networks.push_back(network);
				LOG_INFO("Found known network: \"{:s}\", id: {:d}, current: {:d}",
						 network.ssid.c_str(),
						 network.id,
						 network.connected);
			}
		}
		return networks;
	}

	std::vector<WiFiNetwork> scanWiFiNetworks()
	{
		LOG_INFO("Scanning for WiFi networks");
		std::vector<WiFiNetwork> networks;
		std::vector<WiFiNetwork> knownNetworks = getKnownWiFiNetworks();

		sendCommand("SCAN");
		usleep(100000); // Wait 100 milliseconds for scan to complete

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
					LOG_INFO("Found network: \"{:s}\", signal: {:d} dBm, id: {:d}, {:s}",
							 network.ssid.c_str(),
							 network.signal_level,
							 network.id,
							 network.connected ? "connected" : "disconnected");
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
	}

	bool isNetworkKnown(std::string_view ssid)
	{
		std::vector<WiFiNetwork> networks = getKnownWiFiNetworks();
		return std::any_of(
			networks.begin(), networks.end(), [&ssid](const WiFiNetwork& network) { return network.ssid == ssid; });
	}

	void connect(std::string_view ssid)
	{
		LOG_INFO("Connecting to WiFi network \"{:s}\"", ssid);
		std::vector<WiFiNetwork> networks = getKnownWiFiNetworks();
		for (const WiFiNetwork& network : networks)
		{
			if (network.ssid == ssid)
			{
				std::string cmd = fmt::format("SELECT_NETWORK {}", network.id);
				sendCommand(cmd);
				return;
			}
		}
		LOG_ERROR("Network \"{:s}\" not found in known networks", ssid);
	}

	void connect(std::string_view ssid, std::string_view password)
	{
		LOG_INFO("Connecting to WiFi network \"{:s}\"", ssid);
		if (!isNetworkKnown(ssid))
		{
			std::string cmd = "ADD_NETWORK";
			std::string output = sendCommand(cmd);
			int networkId = std::stoi(output);

			cmd = fmt::format("SET_NETWORK {} ssid \"{}\"", networkId, ssid);
			sendCommand(cmd);

			cmd = fmt::format("SET_NETWORK {} psk \"{}\"", networkId, password);
			sendCommand(cmd);

			cmd = fmt::format("ENABLE_NETWORK {}", networkId);
			sendCommand(cmd);

			sendCommand("SAVE_CONFIG");
		}
		connect(ssid);
	}

	void disconnect()
	{
		LOG_INFO("Disconnecting from WiFi network");
		sendCommand("DISCONNECT");
	}

	void reconnect()
	{
		LOG_INFO("Reconnecting to WiFi network");
		sendCommand("RECONNECT");
	}

	void forgetNetwork(std::string_view ssid)
	{
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
/*
 * NetworkHelper.h
 *
 *  Created on: 2025-01-30
 *      Author: Andy Everitt
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

struct WiFiNetwork
{
	std::string ssid;
	int32_t signal_level = 0;
	int32_t id = -1;
	bool connected = false;

	bool isKnown() const { return id != -1; }

	void clear()
	{
		ssid.clear();
		signal_level = 0;
		id = -1;
		connected = false;
	}
};

namespace NetworkHelper
{
	/// Result of a connection attempt.
	enum class ConnectResult
	{
		Success,
		AuthFailure,	 ///< Wrong password or authentication rejected
		Timeout,		 ///< Association did not complete within the deadline
		NetworkNotFound, ///< SSID not found in known networks
		Error,			 ///< Generic / unexpected failure
	};

	/// Return a human-readable key suffix for ConnectResult (for i18n lookup).
	[[nodiscard]] constexpr std::string_view connectResultKey(ConnectResult r) noexcept
	{
		switch (r)
		{
		case ConnectResult::Success:
			return "success";
		case ConnectResult::AuthFailure:
			return "auth_failure";
		case ConnectResult::Timeout:
			return "timeout";
		case ConnectResult::NetworkNotFound:
			return "network_not_found";
		case ConnectResult::Error:
			return "error";
		}
		return "error";
	}

	/// Enable or disable the WiFi interface.
	/// When enabling, the blocking work (bringing the link up, starting
	/// wpa_supplicant, polling for readiness) runs on an internal thread so
	/// the caller is never blocked.  The optional @p callback is invoked from
	/// the worker thread once the operation completes (`true` on success).
	/// If an enable is already in progress, no new attempt is started and
	/// @p callback is invoked when the in-progress attempt completes.
	/// Disabling is fast and runs synchronously; the callback (if any) is
	/// still invoked before returning.
	void enable(bool enable, std::function<void(bool /*success*/)> callback = nullptr);

	/// Check whether wpa_supplicant reports wpa_state=COMPLETED.
	[[nodiscard]] bool isEnabled();

	void reconfigure();

	[[nodiscard]] std::string getIpAddress();
	[[nodiscard]] std::vector<WiFiNetwork> getKnownWiFiNetworks();
	[[nodiscard]] std::vector<WiFiNetwork> scanWiFiNetworks();

	/// Connect to a known network by SSID. Returns the outcome.
	[[nodiscard]] ConnectResult connect(std::string_view ssid);

	/// Connect to a network with a password. Returns the outcome.
	[[nodiscard]] ConnectResult connect(std::string_view ssid, std::string_view password);

	[[nodiscard]] bool isNetworkKnown(std::string_view ssid);

	void disconnect();
	void reconnect();
	void forgetNetwork(std::string_view ssid);
} // namespace NetworkHelper

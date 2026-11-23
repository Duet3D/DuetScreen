/*
 * WifiPresenter.cpp
 */

#include "WifiPresenter.h"
#include "Debug.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI
{
	namespace
	{
		/** How strong a signal is, from its level in dBm: 4 bars from -55, 3 from -65, 2 from -75, else 1 */
		int bars(int32_t dbm)
		{
			return dbm >= -55 ? 4 : dbm >= -65 ? 3 : dbm >= -75 ? 2 : 1;
		}

		/** The translation keys of the signal's strength, by bars, from 1 */
		constexpr const char* SIGNAL_WORDS[] = {"settings.network.signal_weak",
												"settings.network.signal_fair",
												"settings.network.signal_good",
												"settings.network.signal_excellent"};
	} // namespace

	void WifiNetworkRow::fields(Xml::FieldVisitor& f)
	{
		f.field("ssid", ssid, "Network name");
		f.field("signal", signal, "How strong its signal is, e.g. \"Good (-61 dBm)\"");
		f.field("signal_icon", signalIcon, "Its signal as bars: wifi_signal_1.png to wifi_signal_4.png");
		f.field("state", state, "\"Connected\", \"Saved\" (joined before), or empty");
		f.field("known", known, "1 if the screen has joined it before, so it can be forgotten");
		f.field("connected", connected, "1 if the screen is on it");
		f.field("secured", secured, "1 if joining it takes a password");
	}

	WifiPresenter::WifiPresenter()
		: XmlPresenter("settings_wifi")
	{
		action("wifi.refresh", "Scan for networks now", [this](const Xml::ActionArgs&) { m_scanner.scanNow(); });
		action("wifi.open",
			   "Open the network in the dialog. From a row of `networks`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<WifiNetworkRow>())
				   {
					   open(*row);
				   }
				   else
				   {
					   LOG_WARN("{:s} must come from a networks row", args.name);
				   }
			   });
		action("wifi.connect",
			   "Connect to the network open, which the screen has joined before",
			   [this](const Xml::ActionArgs&) { connect(std::string(m_ssid.get()), std::nullopt); });
		action(
			"wifi.join",
			"Join the network open, with the password typed if it is secured",
			[this](const Xml::ActionArgs&)
			{ connect(std::string(m_ssid.get()), m_passwordShown.get() != 0 ? std::string(m_password.get()) : ""); });
		action("wifi.disconnect",
			   "Leave the network open",
			   [this](const Xml::ActionArgs&)
			   {
				   m_open.set(0);
				   NetworkHelper::disconnect();
				   m_status.set(_("settings.network.status_disconnected"));
				   m_scanner.scanNow();
			   });
		action("wifi.forget",
			   "Forget the network open",
			   [this](const Xml::ActionArgs&)
			   {
				   m_open.set(0);
				   NetworkHelper::forgetNetwork(m_ssid.get());
				   m_scanner.scanNow();
			   });
		action("wifi.password",
			   "Store the text edited (`value_changed` on the password's text box) as the password",
			   [this](const Xml::ActionArgs& args) { m_password.set(args.text()); });
		m_ip.set("-");
	}

	void WifiPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("ip", m_ip, "The screen's IP address, or \"-\"");
		f.field("status", m_status, "How the last connection attempt went, or empty");
		f.list("networks", m_networks, "The networks found");
		f.field("network.open", m_open, "1 while a network is open in the dialog");
		f.field("network.ssid", m_ssid, "The network open");
		f.field("network.signal", m_signal, "Its signal, e.g. \"Good (-61 dBm)\"");
		f.field("network.security", m_security, "\"Secured\" or \"Open\"");
		f.field("network.known", m_known, "1 if the screen has joined it before: it can be connected to or forgotten");
		f.field("network.connected", m_connected, "1 if the screen is on it: it can be left");
		f.field("network.password_shown", m_passwordShown, "1 if joining it asks for a password");
		f.field("network.password", m_password, "The password typed");
	}

	void WifiPresenter::onInit()
	{
		registerEventListener<EventType::WifiNetworks>([this]() { showNetworks(); });
		registerEventListener<EventType::WifiConnectResult>(
			[this](const std::string& ssid, NetworkHelper::ConnectResult result) { showResult(ssid, result); });
	}

	void WifiPresenter::onActivate()
	{
		m_scanner.setScanning(true);
	}

	void WifiPresenter::onDeactivate()
	{
		m_scanner.setScanning(false);
	}

	void WifiPresenter::showNetworks()
	{
		ZoneScoped;
		const std::vector<WiFiNetwork> networks = m_scanner.networks();
		UI_LOCK();
		m_networks.sync(
			networks.size(),
			[&networks](WifiNetworkRow& row, size_t i)
			{
				const WiFiNetwork& network = networks[i];
				const int strength = bars(network.signal_level);
				row.ssid.set(network.ssid);
				row.signal.set(_("settings.network.signal_value", _(SIGNAL_WORDS[strength - 1]), network.signal_level));
				row.signalIcon.set(fmt::format("wifi_signal_{:d}.png", strength));
				row.state.set(network.connected	  ? _("settings.network.connected")
							  : network.isKnown() ? _("settings.network.saved")
												  : std::string());
				row.known.set(network.isKnown() ? 1 : 0);
				row.connected.set(network.connected ? 1 : 0);
				row.secured.set(network.secured ? 1 : 0);
			});
		m_ip.set(NetworkHelper::getIpAddress());
	}

	void WifiPresenter::showResult(const std::string& ssid, NetworkHelper::ConnectResult result)
	{
		UI_LOCK();
		switch (result)
		{
		case NetworkHelper::ConnectResult::Success:
			m_status.set(_("settings.network.status_connected", ssid));
			break;
		case NetworkHelper::ConnectResult::AuthFailure:
			m_status.set(_("settings.network.status_auth_failure", ssid));
			break;
		case NetworkHelper::ConnectResult::Timeout:
			m_status.set(_("settings.network.status_timeout", ssid));
			break;
		case NetworkHelper::ConnectResult::NetworkNotFound:
			m_status.set(_("settings.network.status_not_found", ssid));
			break;
		case NetworkHelper::ConnectResult::Error:
			m_status.set(_("settings.network.status_error", ssid));
			break;
		}
	}

	void WifiPresenter::open(const WifiNetworkRow& row)
	{
		m_ssid.set(row.ssid.get());
		m_signal.set(row.signal.get());
		m_security.set(row.secured.get() != 0 ? _("settings.network.secured") : _("settings.network.open"));
		m_known.set(row.known.get());
		m_connected.set(row.connected.get());
		m_passwordShown.set(row.known.get() == 0 && row.secured.get() != 0 ? 1 : 0);
		m_password.set("");
		m_open.set(1);
	}

	void WifiPresenter::connect(const std::string& ssid, std::optional<std::string> password)
	{
		m_open.set(0);
		m_status.set(_("settings.network.status_connecting", ssid));
		m_scanner.connect(ssid, std::move(password));
	}
} // namespace UI

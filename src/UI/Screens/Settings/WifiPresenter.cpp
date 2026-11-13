/*
 * WifiPresenter.cpp
 */

#include "WifiPresenter.h"
#include "Debug.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI
{
	void WifiNetworkRow::fields(Xml::FieldVisitor& f)
	{
		f.field("ssid", ssid, "Network name");
		f.field("signal", signal, "Signal level, e.g. \"75 dBm\"");
		f.field("known", known, "1 if the screen has joined it before");
		f.field("connected", connected, "1 if the screen is on it");
		f.field("connect_text", connectText, "\"Disconnect\", \"Connect\" or \"Join\"");
	}

	WifiPresenter::WifiPresenter()
		: XmlPresenter("wifi")
	{
		action("wifi.refresh", [this](const Xml::ActionArgs&) { m_scanner.scanNow(); });
		action("wifi.connect",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto* row = networkOf(args);
				   if (row == nullptr)
				   {
					   return;
				   }
				   const std::string ssid(row->ssid.get());
				   if (row->connected.get() != 0)
				   {
					   NetworkHelper::disconnect();
					   m_status.set(_("settings.network.status_disconnected"));
					   m_scanner.scanNow();
				   }
				   else if (row->known.get() != 0)
				   {
					   m_status.set(_("settings.network.status_connecting", ssid));
					   m_scanner.connect(ssid);
				   }
				   else
				   {
					   m_joining = ssid;
					   m_password.set("");
					   m_passwordTitle.set(_("settings.network.password_for", ssid));
					   m_passwordOpen.set(1);
				   }
			   });
		action("wifi.forget",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = networkOf(args))
				   {
					   NetworkHelper::forgetNetwork(row->ssid.get());
					   m_scanner.scanNow();
				   }
			   });
		action("wifi.password", [this](const Xml::ActionArgs& args) { m_password.set(args.text()); });
		action("wifi.join",
			   [this](const Xml::ActionArgs&)
			   {
				   m_passwordOpen.set(0);
				   m_status.set(_("settings.network.status_connecting", m_joining));
				   m_scanner.connect(m_joining, std::string(m_password.get()));
			   });
		action("wifi.close_password", [this](const Xml::ActionArgs&) { m_passwordOpen.set(0); });
		action("wifi.keyboard",
			   [this](const Xml::ActionArgs& args) { m_keyboard.set(args.argInt().value_or(0) != 0 ? 1 : 0); });
		m_ip.set(_("settings.network.ip_address", "-"));
	}

	void WifiPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("ip", m_ip, "E.g. \"IP Address: 192.168.1.100\"");
		f.field("status", m_status, "How the last connection attempt went, or empty");
		f.list("networks", m_networks, "The networks found");
		f.field("password_open", m_passwordOpen, "1 while asking for a password");
		f.field("password_title", m_passwordTitle, "The question");
		f.field("password", m_password, "The password typed");
		f.field("keyboard", m_keyboard, "1 while the keyboard is up");
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
		m_networks.sync(networks.size(),
						[&networks](WifiNetworkRow& row, size_t i)
						{
							const WiFiNetwork& network = networks[i];
							row.ssid.set(network.ssid);
							row.signal.set(fmt::format("{:d} dBm", network.signal_level));
							row.known.set(network.isKnown() ? 1 : 0);
							row.connected.set(network.connected ? 1 : 0);
							row.connectText.set(network.connected	? _("settings.network.disconnect")
												: network.isKnown() ? _("settings.network.connect")
																	: _("settings.network.join"));
						});
		m_ip.set(_("settings.network.ip_address", NetworkHelper::getIpAddress()));
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

	const WifiNetworkRow* WifiPresenter::networkOf(const Xml::ActionArgs& args)
	{
		const auto* row = args.row<WifiNetworkRow>();
		if (row == nullptr)
		{
			LOG_WARN("{:s} must come from a networks row", args.name);
		}
		return row;
	}
} // namespace UI

/*
 * ConnectionSettings.cpp
 */

#include "ConnectionSettings.h"
#include "Comm/Connection.h"
#include "Comm/UsbMode.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "i18n/i18n.h"
#include <chrono>
#include <string>
#include <vector>

namespace UI::ConnectionSettings
{
	namespace
	{
		/** Connect to the Duet by `type`, setting the USB-C port's mode to suit */
		void connectBy(Comm::CommunicationType type)
		{
			Comm::Connection::get().setType(type);
			switch (type)
			{
			case Comm::CommunicationType::uart:
			case Comm::CommunicationType::usb:
				Comm::setUsbMode(Comm::UsbMode::Host);
				break;
			case Comm::CommunicationType::network:
				Comm::setUsbMode(Comm::UsbMode::InternalWiFi);
				break;
			default:
				break;
			}
		}
	} // namespace

	void registerAll()
	{
		Xml::Settings::add({.name = "connection_method",
							.description = "Index of how the screen connects to the Duet (USB, UART or network); "
										   "choosing one sets the USB-C port's mode to suit",
							.read = [] { return static_cast<int32_t>(Comm::Connection::get().type()); },
							.write = [](int32_t index) { connectBy(static_cast<Comm::CommunicationType>(index)); },
							.options =
								[]
							{
								std::vector<std::string> names;
								for (const auto& method : Comm::communicationTypeNames)
								{
									names.push_back(_(method.data()));
								}
								return Xml::Settings::lines(names);
							}});
		Xml::Settings::add(
			{.name = "network_shown",
			 .description = "1 if the Duet is reached over the network, so its IP address and password "
							"apply",
			 .read = [] { return Comm::Connection::get().type() == Comm::CommunicationType::network ? 1 : 0; }});
		Xml::Settings::add(
			{.name = "simulator_in_use",
			 .description = "1 if the Duet connected to is the simulated one, so its page applies",
			 .read = [] { return Comm::Connection::get().type() == Comm::CommunicationType::simulator ? 1 : 0; }});
		Xml::Settings::add({.name = "usb_mode",
							.description = "Index of the USB-C port's mode",
							.read = [] { return static_cast<int32_t>(Comm::getUsbMode()); },
							.write = [](int32_t index) { Comm::setUsbMode(Comm::UsbMode(index)); },
							.options =
								[]
							{
								return Xml::Settings::lines({_("settings.usb_mode_host"),
															 _("settings.usb_mode_device"),
															 _("settings.usb_mode_internal_wifi")});
							}});
		Xml::Settings::add(
			{.name = "poll_interval",
			 .description = "ms between polls of the Duet",
			 .read = [] { return static_cast<int32_t>(Comm::Connection::get().pollInterval().count()); },
			 .write = [](int32_t ms) { Comm::Connection::get().setPollInterval(std::chrono::milliseconds(ms)); }});
		Xml::Settings::add(Xml::TextSetting{.name = "duet_ip",
											.description = "The Duet's IP address",
											.read = [] { return std::string(Comm::Connection::get().ipAddress()); },
											.write = [](std::string_view ip)
											{ Comm::Connection::get().setIpAddress(std::string(ip)); },
											.askHeader = [] { return _("settings.duet_ip_address_prompt"); }});
		Xml::Settings::add(Xml::TextSetting{.name = "duet_password",
											.description = "The Duet's password",
											.read = [] { return std::string(Comm::Connection::get().password()); },
											.write = [](std::string_view password)
											{ Comm::Connection::get().setPassword(std::string(password)); }});
	}
} // namespace UI::ConnectionSettings

/*
 * ConnectionSettings.cpp
 */

#include "ConnectionSettings.h"
#include "Comm/Usb.h"
#include "Hardware/Duet.h"
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
			Comm::DUET.SetCommunicationType(type);
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
							.read = [] { return static_cast<int32_t>(Comm::DUET.GetCommunicationType()); },
							.write = [](int32_t index) { connectBy(static_cast<Comm::CommunicationType>(index)); },
							.options =
								[]
							{
								std::vector<std::string> names;
								for (const auto& method : Comm::duetCommunicationTypeNames)
								{
									names.push_back(_(method.data()));
								}
								return Xml::Settings::lines(names);
							}});
		Xml::Settings::add(
			{.name = "network_shown",
			 .description = "1 if the Duet is reached over the network, so its IP address and password "
							"apply",
			 .read = [] { return Comm::DUET.GetCommunicationType() == Comm::CommunicationType::network ? 1 : 0; }});
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
		Xml::Settings::add({.name = "poll_interval",
							.description = "ms between polls of the Duet",
							.read = [] { return static_cast<int32_t>(Comm::DUET.GetPollInterval().count()); },
							.write = [](int32_t ms) { Comm::DUET.SetPollInterval(std::chrono::milliseconds(ms)); }});
		Xml::Settings::add(
			Xml::TextSetting{.name = "duet_ip",
							 .description = "The Duet's IP address",
							 .read = [] { return std::string(Comm::DUET.GetIPAddress()); },
							 .write = [](std::string_view ip) { Comm::DUET.SetIPAddress(std::string(ip)); },
							 .askHeader = [] { return _("settings.duet_ip_address_prompt"); }});
		Xml::Settings::add(Xml::TextSetting{.name = "duet_password",
											.description = "The Duet's password",
											.read = [] { return std::string(Comm::DUET.GetPassword()); },
											.write = [](std::string_view password)
											{ Comm::DUET.SetPassword(std::string(password)); }});
	}
} // namespace UI::ConnectionSettings

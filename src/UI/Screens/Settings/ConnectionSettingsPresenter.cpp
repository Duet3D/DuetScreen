/*
 * ConnectionSettingsPresenter.cpp
 */

#include "ConnectionSettingsPresenter.h"
#include "Comm/Usb.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"

namespace UI
{
	ConnectionSettingsPresenter::ConnectionSettingsPresenter()
		: XmlPresenter("settings_connection")
	{
		std::string methods;
		for (const auto& method : Comm::duetCommunicationTypeNames)
		{
			methods += methods.empty() ? "" : "\n";
			methods += _(method.data());
		}
		m_methods.set(methods);
		m_usbModes.set(fmt::format("{}\n{}\n{}",
								   _("settings.usb_mode_host"),
								   _("settings.usb_mode_device"),
								   _("settings.usb_mode_internal_wifi")));

		action("settings.connection_method",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto value = args.value();
				   if (!value)
				   {
					   return;
				   }
				   const auto type = static_cast<Comm::CommunicationType>(static_cast<int>(*value));
				   Comm::DUET.SetCommunicationType(type);
				   // The USB-C port is used the way the connection needs
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
				   showMethod();
			   });
		action("settings.usb_mode",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto value = args.value())
				   {
					   Comm::setUsbMode(Comm::UsbMode(static_cast<int>(*value)));
					   m_usbMode.set(static_cast<int32_t>(*value));
				   }
			   });
		action("settings.poll_interval",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto value = args.value())
				   {
					   m_pollInterval.set(static_cast<int32_t>(*value));
					   Comm::DUET.SetPollInterval(std::chrono::milliseconds(static_cast<int32_t>(*value)));
				   }
			   });
		action("settings.duet_ip",
			   [this](const Xml::ActionArgs& args)
			   {
				   Xml::Services::askText(args.target,
										  {.header = _("settings.duet_ip_address_prompt"),
										   .text = std::string(m_ip.get()),
										   .onConfirm = [this](std::string_view ip)
										   {
											   m_ip.set(ip);
											   Comm::DUET.SetIPAddress(std::string(ip));
										   }});
			   });
		action("settings.duet_password",
			   [this](const Xml::ActionArgs& args)
			   {
				   const std::string password = args.text();
				   m_password.set(password);
				   Comm::DUET.SetPassword(password);
			   });
	}

	void ConnectionSettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("connection_methods", m_methods, "Connection methods offered, one per line");
		f.field("connection_method", m_method, "Index of the one in use");
		f.field("usb_modes", m_usbModes, "USB-C modes offered, one per line");
		f.field("usb_mode", m_usbMode, "Index of the one in use");
		f.field("poll_interval", m_pollInterval, "ms between polls of the Duet");
		f.field("network_shown", m_networkShown, "1 if the Duet is reached over the network");
		f.field("duet_ip", m_ip, "The Duet's IP address");
		f.field("duet_password", m_password, "The Duet's password");
	}

	void ConnectionSettingsPresenter::onActivate()
	{
		showMethod();
		m_pollInterval.set(static_cast<int32_t>(Comm::DUET.GetPollInterval().count()));
		m_ip.set(Comm::DUET.GetIPAddress());
		m_password.set(Comm::DUET.GetPassword());
	}

	void ConnectionSettingsPresenter::showMethod()
	{
		const auto type = Comm::DUET.GetCommunicationType();
		m_method.set(static_cast<int32_t>(type));
		m_usbMode.set(static_cast<int32_t>(Comm::getUsbMode()));
		m_networkShown.set(type == Comm::CommunicationType::network ? 1 : 0);
	}
} // namespace UI

/*
 * ConnectionSettingsPresenter.h
 *
 * Behaviour for the Connection settings tab (assets/ui/screens/settings_connection.xml): how the screen talks to the
 * Duet. The screen's own WiFi is wifi_selector.xml (WifiPresenter.h). Each setting is read when the tab is shown and
 * applied as soon as it is changed.
 *
 * Fields
 *   connection_methods  USB, UART or network, one per line; connection_method is the one in use
 *   usb_modes           the USB-C port's modes, one per line; usb_mode is the one in use
 *   poll_interval       ms between polls of the Duet
 *   network_shown       1 if the Duet is reached over the network, so its IP address and password are shown
 *   duet_ip             the Duet's IP address
 *   duet_password       the Duet's password
 *
 * Actions
 *   settings.connection_method, settings.usb_mode   (an index) use that one
 *   settings.poll_interval                          (a number) poll that often
 *   settings.duet_ip                                ask for the IP address on the nearest number pad
 *   settings.duet_password                          (the text, from its text box) use that password
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class ConnectionSettingsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		ConnectionSettingsPresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the settings as they are now */
		void onActivate() override;

	  private:
		/** Show the connection method and USB mode in use, and whether the network settings apply */
		void showMethod();

		Xml::StringSubject<128> m_methods;	   // Connection methods offered
		Xml::IntSubject m_method;			   // Index of the one in use
		Xml::StringSubject<128> m_usbModes;	   // USB modes offered
		Xml::IntSubject m_usbMode;			   // Index of the one in use
		Xml::IntSubject m_pollInterval;		   // ms
		Xml::IntSubject m_networkShown;		   // 1 over the network
		Xml::StringSubject<64> m_ip;		   // Duet IP address
		Xml::StringSubject<64> m_password;	   // Duet password
	};
} // namespace UI

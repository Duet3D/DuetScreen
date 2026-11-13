/*
 * ConnectionSettingsPresenter.h
 *
 * Behaviour for the Connection settings tab (assets/ui/components/settings/settings_connection.xml): how the screen
 * talks to the Duet. The screen's own WiFi is settings_wifi.xml (WifiPresenter.h). Each setting is read when the tab is
 * shown and applied as soon as it is changed.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
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

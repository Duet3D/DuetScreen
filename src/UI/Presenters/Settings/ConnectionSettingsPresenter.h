/*
 * ConnectionSettingsPresenter.h
 *
 * Behaviour for the Connection settings page (assets/ui/components/settings/settings_connection.xml): how the screen
 * talks to the Duet. The USB-C port's mode and the poll interval show only with advanced settings shown (Developer).
 * The screen's own WiFi is settings_wifi.xml (WifiPresenter.h). Each setting is read when the page is shown and
 * applied as soon as it is changed.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "SettingsPresenter.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI
{
	class ConnectionSettingsPresenter : public SettingsPresenter
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
		Xml::IntSubject m_advancedShown;	   // 1 if advanced settings are shown (Developer > Show advanced settings)
		Xml::StringSubject<64> m_ip;		   // Duet IP address
		Xml::StringSubject<64> m_password;	   // Duet password
	};
} // namespace UI

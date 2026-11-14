/*
 * WifiPresenter.h
 *
 * Behaviour for the screen's WiFi (assets/ui/components/settings/settings_wifi.xml): its IP address, how the last
 * connection attempt went, and the networks around it, scanned for every few seconds while the view is shown (see
 * WifiScanner.h). Tapping a network opens it in a dialog, which connects to it, leaves it or forgets it, or, for one
 * the screen hasn't joined, joins it, asking for its password if it is secured.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "WifiScanner.h"
#include <optional>
#include <string>

namespace UI
{
	/** One network found */
	class WifiNetworkRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> ssid;	   // Network name
		Xml::StringSubject<32> signal;	   // How strong its signal is, e.g. "Good (-61 dBm)"
		Xml::StringSubject<32> signalIcon; // Its signal as bars, an icon from the icon set
		Xml::StringSubject<32> state;	   // "Connected", "Saved" (joined before) or empty
		Xml::IntSubject known;			   // 1 if joined before
		Xml::IntSubject connected;		   // 1 if the screen is on it
		Xml::IntSubject secured;		   // 1 if joining it takes a password
	};

	class WifiPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		WifiPresenter();

		/** Declare the fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for scans and connection attempts */
		void onInit() override;

		/** Scan every few seconds */
		void onActivate() override;

		/** Stop scanning */
		void onDeactivate() override;

	  private:
		/** Show the networks the last scan found, and the IP address */
		void showNetworks();

		/** Show how connecting to `ssid` went */
		void showResult(const std::string& ssid, NetworkHelper::ConnectResult result);

		/** Open the network of `row` in the dialog */
		void open(const WifiNetworkRow& row);

		/** Close the dialog, and start connecting to `ssid` (with `password`, to join it) */
		void connect(const std::string& ssid, std::optional<std::string> password);

		WifiScanner m_scanner; // Scans and connects

		Xml::StringSubject<64> m_ip;		  // The screen's IP address
		Xml::StringSubject<128> m_status;	  // How the last attempt went, or empty
		Xml::List<WifiNetworkRow> m_networks; // The networks found

		Xml::IntSubject m_open;					// 1 while a network is open in the dialog
		Xml::StringSubject<64> m_ssid;			// The network open
		Xml::StringSubject<32> m_signal;		// Its signal, as its row says
		Xml::StringSubject<32> m_security;		// "Secured" or "Open"
		Xml::IntSubject m_known;				// 1 if the screen has joined it before
		Xml::IntSubject m_connected;			// 1 if the screen is on it
		Xml::IntSubject m_passwordShown;		// 1 if joining it asks for a password
		Xml::StringSubject<128> m_password;		// The password typed
	};
} // namespace UI

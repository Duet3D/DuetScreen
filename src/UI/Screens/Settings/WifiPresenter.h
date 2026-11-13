/*
 * WifiPresenter.h
 *
 * Behaviour for the screen's WiFi networks (assets/ui/components/wifi_selector.xml): the networks around it, joining,
 * leaving and forgetting them, and its IP address. Networks are scanned for every few seconds while the view is shown
 * (see WifiScanner.h).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "WifiScanner.h"
#include <string>

namespace UI
{
	/** One WiFi network */
	class WifiNetworkRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> ssid;		 // Network name
		Xml::StringSubject<16> signal;		 // "75 dBm"
		Xml::IntSubject known;				 // 1 if joined before
		Xml::IntSubject connected;			 // 1 if the screen is on it
		Xml::StringSubject<32> connectText;	 // What its connect button does
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

		/** The network an action's row is about, or nullptr */
		static const WifiNetworkRow* networkOf(const Xml::ActionArgs& args);

		WifiScanner m_scanner;	   // Scans and connects
		std::string m_joining;	   // The network whose password is asked for

		Xml::StringSubject<64> m_ip;			   // "IP Address: …"
		Xml::StringSubject<128> m_status;		   // The last attempt's result
		Xml::List<WifiNetworkRow> m_networks;	   // The networks found
		Xml::IntSubject m_passwordOpen;			   // 1 while asking for a password
		Xml::StringSubject<96> m_passwordTitle;	   // The question
		Xml::StringSubject<128> m_password;		   // The password typed
		Xml::IntSubject m_keyboard;				   // 1 while the keyboard is up
	};
} // namespace UI

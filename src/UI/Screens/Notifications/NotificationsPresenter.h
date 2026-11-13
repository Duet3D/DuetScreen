/*
 * NotificationsPresenter.h
 *
 * Behaviour for the notifications over every screen (assets/ui/components/notifications.xml): responses from the
 * Duet, M291 alerts, and firmware update prompts.
 *
 * Responses are queued and shown one at a time, each closing itself after the notification timeout. They wait while
 * an alert is open, and aren't shown at all while the console (which shows them itself) is open.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "ObjectModel/Alert.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "utils/UpgradeHelper.h"
#include "lvgl/lvgl.h"
#include <chrono>
#include <deque>
#include <string>

namespace UI
{
	/** One choice in an alert */
	class AlertChoiceRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<64> text; // The choice
	};

	/** One axis an alert lets the user jog */
	class AlertAxisRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<24> label; // E.g. "X = 0.00"
		Xml::IntSubject homed;		  // 1 if the axis is homed, so it can be jogged
		char letter = 0;			  // The axis letter; not shown
	};

	class NotificationsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		NotificationsPresenter();

		/** Stop the countdown timer */
		~NotificationsPresenter() override;

		/** Declare the notifications' fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Queue a response from the Duet, unless the console is showing or it is below the notification level */
		void newResponse(const ResponseType type, const std::string& response);

		/** Show, update or close the M291 alert */
		void newAlertData(const OM::Alert& alert);

		/** An update file was found on USB: offer it */
		void newUpdateAvailable(const std::string& file);

		/** A newer release is on GitHub: offer it */
		void newGithubUpdateAvailable(const std::string& latestVersion);

		/** Report how an update went */
		void handleUpdateResult(const UpgradeHelper::UpgradeInfo& info);

	  protected:
		/** Listen for responses, alerts, updates, axes and USB drives */
		void onInit() override;

		/** Show the alert's axes as they are now */
		void onActivate() override;

		/** Forget everything from the printer */
		void onDisconnect() override;

	  private:
		/** A response waiting to be shown */
		struct Response
		{
			ResponseType type; // How serious it is
			std::string text;  // What it says
		};

		/** A self-closing message's time */
		struct Countdown
		{
			uint32_t start = 0;	   // lv_tick when it started
			uint32_t duration = 0; // ms; 0 for none
		};

		/** Show the first queued response, unless the alert is open, and start its countdown */
		void showFirstResponse();

		/** Close the response showing and show the next */
		void closeResponse();

		/** Close the alert and show any waiting responses */
		void closeAlert();

		/** Check the value typed into the alert, enabling OK and showing why it isn't valid */
		void validateInput(std::string_view text);

		/** Show the alert's axes' positions and whether they can move */
		void updateAlertAxes();

		/** Offer an update: OK runs `install` */
		void offerUpdate(std::string text, std::function<void()> install);

		/** Countdown timer: show the time left, and close what has run out */
		static void tickCb(lv_timer_t* timer);

		/** Percentage of `countdown` left; 0 if it has none */
		static int32_t remaining(const Countdown& countdown);

		std::deque<Response> m_responses; // Queued responses; the first is showing (unless the alert is open)
		Countdown m_responseCountdown;	  // The showing response's countdown
		Countdown m_alertCountdown;		  // The alert's countdown
		lv_timer_t* m_timer = nullptr;	  // Drives the countdowns

		OM::Alert m_alert;				  // The alert showing
		std::string m_input;			  // The value typed into the alert
		bool m_cancelSends = false;		  // Cancel answers the printer (the alert offers it)
		std::string m_updateFile;		  // Update file offered from USB
		std::function<void()> m_install;  // What update.confirm does

		Xml::IntSubject m_responseOpen;				// 1 while a response shows
		Xml::StringSubject<32> m_responseTitle;		// Its title
		Xml::StringSubject<1024> m_responseText;	// Its text
		Xml::IntSubject m_responseType;				// Its type
		Xml::IntSubject m_responseProgress;			// Its time left, 0-100

		Xml::IntSubject m_updateOpen;				// 1 while the update prompt shows
		Xml::StringSubject<64> m_updateTitle;		// Its title
		Xml::StringSubject<512> m_updateText;		// Its text
		Xml::StringSubject<32> m_updateOkText;		// Its OK button text
		Xml::IntSubject m_updateCanCancel;			// 1 if it can be declined

		Xml::IntSubject m_alertOpen;				// 1 while the alert shows
		Xml::StringSubject<64> m_alertTitle;		// Its title
		Xml::StringSubject<256> m_alertText;		// Its text
		Xml::IntSubject m_alertOk;					// OK: 0 hidden, 1 disabled, 2 usable
		Xml::StringSubject<32> m_alertOkText;		// OK's text
		Xml::IntSubject m_alertCancel;				// Cancel: 0 hidden, 2 usable
		Xml::IntSubject m_alertFooterShown;			// 1 if OK or Cancel shows
		Xml::List<AlertChoiceRow> m_alertChoices;	// Choices offered
		Xml::IntSubject m_alertChoicesShown;		// 1 if choices are offered
		Xml::List<AlertAxisRow> m_alertAxes;		// Axes to jog
		Xml::IntSubject m_alertAxesShown;			// 1 if there are axes to jog
		Xml::IntSubject m_alertInputShown;			// 1 if a value is asked for
		Xml::StringSubject<256> m_alertInput;		// The value to start with
		Xml::StringSubject<16> m_alertAccepted;		// Characters the value may contain
		Xml::StringSubject<8> m_alertKeyboardMode;	// "number" or "text"
		Xml::StringSubject<24> m_alertMin;			// Lower limit text, or empty
		Xml::StringSubject<24> m_alertMax;			// Upper limit text, or empty
		Xml::StringSubject<96> m_alertWarning;		// Why the value isn't valid, or empty
		Xml::IntSubject m_alertKeyboard;			// 1 while the keyboard is up
		Xml::IntSubject m_alertProgress;			// Time left, 0-100
		Xml::IntSubject m_alertProgressShown;		// 1 if the alert closes itself
	};
} // namespace UI

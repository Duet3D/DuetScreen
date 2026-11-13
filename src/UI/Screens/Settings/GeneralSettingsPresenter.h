/*
 * GeneralSettingsPresenter.h
 *
 * Behaviour for the General settings tab (assets/ui/screens/settings_general.xml). Each setting is read when the tab
 * is shown and stored (and applied) as soon as it is changed.
 *
 * Fields
 *   firmware, buildroot         the versions, as text
 *   languages                   the languages, one per line; language is the index of the one in use
 *   keyboard_layouts            the USB keyboard layouts, one per line; keyboard_layout is the one in use
 *   brightness                  0-100
 *   screensaver_timeout         seconds
 *   confirmations               1 if actions are confirmed first
 *   machine_position_mode       1 if absolute moves use machine coordinates (G53)
 *   progress_sources            how a job's progress is worked out, one per line; progress_source is the one used
 *   connected_message           1 if a message is shown on connecting
 *   notification_levels         the levels, one per line; notification_level is the lowest shown
 *   notification_timeout        ms before a notification closes itself
 *   error_notifications_stay    1 if error notifications stay until closed
 *   language_confirm            1 while asking whether to restart in another language
 *   language_confirm_title, language_confirm_text   that question
 *
 * Actions (the value from the widget: an index, a number, or 1/0 for a toggle)
 *   settings.language, settings.keyboard_layout, settings.brightness, settings.screensaver_timeout,
 *   settings.confirmations, settings.machine_position_mode, settings.progress_source, settings.connected_message,
 *   settings.notification_level, settings.notification_timeout, settings.error_notifications_stay
 *   settings.restart_in_language    answer yes: store the language picked and restart
 *   settings.keep_language          answer no: keep the language in use
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>
#include <vector>

namespace UI
{
	class GeneralSettingsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		GeneralSettingsPresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the settings as they are now */
		void onActivate() override;

	  private:
		/** Register an action that stores the value the widget gives with `store` */
		void setting(std::string name, std::function<void(float value)> store);

		/** Ask whether to restart in the language at `index` */
		void askLanguage(size_t index);

		std::vector<std::string> m_languageCodes; // Language codes, in the order offered
		std::string m_pickedLanguage;			  // The language asked about

		Xml::StringSubject<96> m_firmware;				   // Firmware version
		Xml::StringSubject<64> m_buildroot;				   // Buildroot version
		Xml::StringSubject<512> m_languages;			   // Languages offered
		Xml::IntSubject m_language;						   // Index of the language in use
		Xml::StringSubject<512> m_keyboardLayouts;		   // Keyboard layouts offered
		Xml::IntSubject m_keyboardLayout;				   // Index of the layout in use
		Xml::IntSubject m_brightness;					   // 0-100
		Xml::IntSubject m_screensaverTimeout;			   // Seconds
		Xml::IntSubject m_confirmations;				   // 1 if actions are confirmed
		Xml::IntSubject m_machinePositionMode;			   // 1 for machine coordinates
		Xml::StringSubject<128> m_progressSources;		   // Progress sources offered
		Xml::IntSubject m_progressSource;				   // Index of the source used
		Xml::IntSubject m_connectedMessage;				   // 1 if shown on connecting
		Xml::StringSubject<128> m_notificationLevels;	   // Levels offered
		Xml::IntSubject m_notificationLevel;			   // Index of the lowest level shown
		Xml::IntSubject m_notificationTimeout;			   // ms
		Xml::IntSubject m_errorNotificationsStay;		   // 1 if errors stay until closed
		Xml::IntSubject m_languageConfirm;				   // 1 while asking
		Xml::StringSubject<64> m_languageConfirmTitle;	   // The question's title
		Xml::StringSubject<256> m_languageConfirmText;	   // The question
	};
} // namespace UI

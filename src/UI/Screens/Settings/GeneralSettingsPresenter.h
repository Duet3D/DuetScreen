/*
 * GeneralSettingsPresenter.h
 *
 * Behaviour for the General settings tab (assets/ui/screens/settings_general.xml). Each setting is read when the tab
 * is shown and stored (and applied) as soon as it is changed.
 *
 * There is one action per setting, given the value from the widget (an index, a number, or 1/0 for a toggle), and
 * the answers to the language question.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
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
		/** Register an action, described by `description`, that stores the value the widget gives with `store` */
		void setting(std::string name, std::string description, std::function<void(float value)> store);

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

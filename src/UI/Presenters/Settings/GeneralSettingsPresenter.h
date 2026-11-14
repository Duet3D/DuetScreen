/*
 * GeneralSettingsPresenter.h
 *
 * Behaviour for the General settings page (assets/ui/components/settings/settings_general.xml): the language and USB
 * keyboard layout, and how the screen works the machine. Each setting is read when the page is shown and stored (and
 * applied) as soon as it is changed.
 *
 * There is one action per setting, given the value from the widget (an index, a number, or 1/0 for a toggle). Picking
 * another language asks whether to restart in it.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "SettingsPresenter.h"
#include "UI/Xml/Binding/Subjects.h"
#include <string>
#include <vector>

namespace UI
{
	class GeneralSettingsPresenter : public SettingsPresenter
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
		/** Ask whether to restart in the language at `index`; the question goes with `asker` */
		void askLanguage(size_t index, lv_obj_t* asker);

		std::vector<std::string> m_languageCodes; // Language codes, in the order offered

		Xml::StringSubject<512> m_languages;			   // Languages offered
		Xml::IntSubject m_language;						   // Index of the language in use
		Xml::StringSubject<512> m_keyboardLayouts;		   // Keyboard layouts offered
		Xml::IntSubject m_keyboardLayout;				   // Index of the layout in use
		Xml::IntSubject m_confirmations;				   // 1 if actions are confirmed
		Xml::IntSubject m_machinePositionMode;			   // 1 for machine coordinates
		Xml::StringSubject<128> m_progressSources;		   // Progress sources offered
		Xml::IntSubject m_progressSource;				   // Index of the source used
	};
} // namespace UI

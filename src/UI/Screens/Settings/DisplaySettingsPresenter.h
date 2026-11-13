/*
 * DisplaySettingsPresenter.h
 *
 * Behaviour for the Display settings tab (assets/ui/screens/settings_display.xml): the theme, font and icons, the theme
 * preview (theme_preview.xml, ThemePreviewPresenter.h), animations and screen rotation. Each setting is read when the
 * tab is shown and applied as soon as it is changed.
 *
 * Fields
 *   themes, fonts, icon_sets, rotations   the choices, one per line
 *   theme, font, icon_set, rotation       the index of the one in use
 *   animations                            1 if the UI animates
 *
 * Actions (the value from the widget: an index, or 1/0 for a toggle)
 *   settings.theme, settings.font, settings.icon_set, settings.rotation, settings.animations
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>
#include <vector>

namespace UI
{
	class DisplaySettingsPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		DisplaySettingsPresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the settings as they are now */
		void onActivate() override;

	  private:
		std::vector<std::string> m_fonts;	  // Typefaces, in the order offered
		std::vector<std::string> m_iconSets;  // Icon sets, in the order offered

		Xml::StringSubject<256> m_themeNames;	 // Themes offered
		Xml::IntSubject m_theme;				 // Index of the one in use
		Xml::StringSubject<512> m_fontNames;	 // Typefaces offered
		Xml::IntSubject m_font;					 // Index of the one in use
		Xml::StringSubject<256> m_iconSetNames;	 // Icon sets offered
		Xml::IntSubject m_iconSet;				 // Index of the one in use
		Xml::StringSubject<128> m_rotations;	 // Rotations offered
		Xml::IntSubject m_rotation;				 // Index of the one in use
		Xml::IntSubject m_animations;			 // 1 if the UI animates
	};
} // namespace UI

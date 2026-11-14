/*
 * DisplaySettingsPresenter.h
 *
 * Behaviour for the Display settings page (assets/ui/components/settings/settings_display.xml): the screen's
 * brightness, screensaver, rotation and animations; the theme, dark mode, font and icons; customising the home screen.
 * The theme's colours are a page of their own (settings_theme_preview.xml, ThemePreviewPresenter.h). Each setting is
 * read when the page is shown and applied as soon as it is changed.
 *
 * There is one action per setting, given the value from the widget (a number, an index, or 1/0 for a switch). Dark
 * mode, for a theme whose palette can be changed, posts ThemeChanged, as changing the theme does; the page shows the
 * theme again whenever one is posted, e.g. after its colours change.
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
	class DisplaySettingsPresenter : public SettingsPresenter
	{
	  public:
		/** Create the presenter */
		DisplaySettingsPresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for theme changes, which change dark mode and the colours shown */
		void onInit() override;

		/** Show the settings as they are now */
		void onActivate() override;

	  private:
		/** Show the theme in use: whether it is in dark mode (and whether that can change), and its accents */
		void showTheme();

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
		Xml::IntSubject m_brightness;			 // 0-100
		Xml::IntSubject m_screensaverTimeout;	 // Seconds; 0 for never
		Xml::IntSubject m_dark;					 // 1 if the theme in use is in dark mode
		Xml::IntSubject m_darkShown;			 // 1 if the theme in use can change between dark and light
		Xml::ColorSubject m_primary;			 // The theme's primary colour
		Xml::ColorSubject m_secondary;			 // The theme's secondary colour
	};
} // namespace UI

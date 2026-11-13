/*
 * ThemePreviewPresenter.h
 *
 * Behaviour for the theme preview (assets/ui/components/settings/theme_preview.xml): a swatch of each colour the theme
 * in use gives its styles, and, for a theme whose palette can be changed, sliders for its hues and chroma and a dark
 * mode toggle, which change it as they move.
 *
 * Actions change the palette as the controls move.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 *
 * Listens for ThemeChanged, which whatever changes the theme posts.
 */

#pragma once

#include "UI/Styles/Palette.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>

namespace UI
{
	/** One colour of the theme */
	class SwatchRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<48> label;	   // The style's name
		Xml::ColorSubject color;		   // Its colour
		Xml::IntSubject textStyle;		   // 1 muted, 2 emphasis, 3 header, else 0
	};

	class ThemePreviewPresenter : public Xml::XmlPresenter
	{
	  public:
		/**
		 * Create the presenter
		 * @param controls Whether the palette controls show for a theme whose palette can be changed; the theme gallery
		 *        shows swatches only
		 */
		explicit ThemePreviewPresenter(bool controls = true);

		/** Declare the preview's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for theme changes */
		void onInit() override;

		/** Show the theme in use */
		void onActivate() override;

	  private:
		/** Show the theme in use: its swatches, and its palette if it can be changed */
		void showTheme();

		/** Show a swatch of each colour */
		void showSwatches();

		/** Show the colours' lightness and chroma, for the hue sliders */
		void showColours();

		/** Change the palette with `change`, and show the result */
		void changePalette(const std::function<void(Themes::PaletteInputs&)>& change);

		bool m_controls; // Whether the palette controls may show

		Xml::List<SwatchRow> m_swatches;	   // The colours
		Xml::IntSubject m_controlsShown;	   // 1 if the palette can be changed here
		Xml::IntSubject m_primaryHue;		   // 0-360
		Xml::IntSubject m_secondaryHue;		   // 0-360
		Xml::IntSubject m_chroma;			   // 0-0.4, in hundredths
		Xml::IntSubject m_dark;				   // 1 in dark mode
		Xml::IntSubject m_primaryLightness;  // Primary colour's lightness, thousandths
		Xml::IntSubject m_primaryChroma;	   // Primary colour's chroma, thousandths
		Xml::IntSubject m_secondaryLightness; // Secondary colour's lightness, thousandths
		Xml::IntSubject m_secondaryChroma;   // Secondary colour's chroma, thousandths
	};
} // namespace UI

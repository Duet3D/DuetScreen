/*
 * Theme.h
 *
 * A theme decides what the global styles (LvglStyles, ComponentStyles in Styles.h) and fonts look like. Themes are
 * XML files (see ThemeFile.h) loaded with the UI bundles; the built-in ones are in assets/ui/themes/.
 *
 * Inheritance
 *   A theme names its parent with `extends`. Building a theme:
 *     1. Palette inputs, colour adjustments, fonts and consts are merged down the chain from the root ancestor, a
 *        theme overriding its parent name by name (attribute by attribute for the palette inputs and fonts).
 *     2. Every style is emptied, then each `<style>` element is applied in order, the root ancestor's file first:
 *        `copy="x"` replaces the style with style x as it is at that point, `merge="x y"` adds those styles'
 *        properties, then the element's own properties are set. So a theme's styles are changes on top of its
 *        parent's, and `copy`/`merge` see whatever the earlier elements made.
 *
 * Values
 *   Property and const values are LVGL's XML style values (`5`, `50%`, `circle`, `0xff0000`, `ver`, …), plus:
 *     #name                 a palette colour (see Palette.h) or a const; `-#name` negates a number
 *     7dpx                  7 pixels at 160 dpi, scaled to the display (LV_DPX); `-7dpx` is its negative
 *     lighten(#c, 0.04)     palette colour c with its OKLCH lightness raised (darken() lowers it)
 *     rgb_darken(#c, 10)    palette colour c mixed towards black (lv_color_darken)
 *
 * Palette colours and consts are also XML consts while the theme is active, so components can use
 * `style_bg_color="#primary"` and follow theme changes.
 */

#pragma once

#include "Palette.h"
#include "ThemeFile.h"
#include "lvgl/lvgl.h"
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Themes
{
	/** Size and FreeType style flags of one font */
	struct FontConfig
	{
		uint32_t size = 14;								   // Pixel size
		uint32_t style = LV_FREETYPE_FONT_STYLE_NORMAL; // LV_FREETYPE_FONT_STYLE_* flags and weight
	};

	/** The four fonts every theme provides */
	struct FontConfigSet
	{
		FontConfig header;	 // Headings
		FontConfig normal;	 // Body text
		FontConfig emphasis; // Emphasised text
		FontConfig subdued;	 // Secondary text
	};

	class Theme
	{
	  public:
		/** A theme called `name`. Its definition is looked up in the loaded theme files when it is used. */
		explicit Theme(std::string name);

		/** The theme's name */
		std::string_view getName() const { return m_name; }

		/** Whether the user can choose the palette's hues, chroma and mode (`<palette customisable="true">`) */
		bool isCustomisable() const;

		/** The palette inputs in use: the user's choice for a customisable theme, else the theme file's */
		PaletteInputs getPalette() const;

		/** The palette inputs the theme file sets */
		PaletteInputs getDefaultPalette() const;

		/** Store the user's palette choice. Call refreshCurrentTheme() to see it. Ignored if not customisable. */
		void setPalette(const PaletteInputs& palette);

		/** Forget the user's palette choice */
		void resetPalette();

		/** The theme's colours with the palette in use */
		ThemeColors getColors() const;

		/** The theme's fonts */
		FontConfigSet getFonts() const;

		/** Build every style and font from this theme and redraw */
		void setThemeActive();

	  private:
		std::string m_name; // Theme name
	};

	/**
	 * Replace the loaded themes. A later file replaces an earlier one with the same name. Then the active theme is
	 * built again, or, the first time, the stored theme (or the default).
	 * @param problems Receives a message for each problem found (unknown parent, unknown style, bad value, …)
	 */
	void setThemeFiles(std::vector<ThemeFile> files, std::vector<std::pair<std::filesystem::path, std::string>>* problems);

	/** Forget every theme and free what the styles point to: styles are emptied and the theme fonts released. Call
	 * before lv_deinit(). */
	void clearThemes();

	/** The themes users can choose (not abstract ones), by name */
	const std::map<std::string_view, Theme*>& getThemes();

	/** The active theme, or nullptr */
	Theme* getCurrentTheme();

	/** The `index`th theme in name order, or nullptr */
	Theme* getTheme(size_t index);

	/** The theme called `name`, or nullptr */
	Theme* getThemeByName(std::string_view name);

	/** The default theme (ID_THEME's default), else the first; nullptr if none are loaded */
	Theme* getDefaultTheme();

	/** Number of themes users can choose */
	size_t getThemeCount();

	/** Build the active theme again, e.g. after setPalette(). @return false if no theme is active */
	bool refreshCurrentTheme();

	/** Names of the themes users can choose, in order */
	std::vector<std::string_view> getThemeNames();

	/** Recreate the theme fonts in `typeface` (see FontManager) */
	void setTypeface(const std::string& typeface);
} // namespace UI::Themes

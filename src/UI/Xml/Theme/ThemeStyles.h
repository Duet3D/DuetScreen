/*
 * ThemeStyles.h
 *
 * Makes the theme's styles and fonts available to XML, and to theme files, by name: every member of LvglStyles and
 * ComponentStyles (`<style name="card"/>`), the four theme fonts (`style_text_font="font_header"`) and the LVGL
 * symbols themes use as images (`bg_image_src="symbol_ok"`).
 *
 * XML uses the theme's own style objects rather than copies, so a theme switch, which rewrites those objects in
 * place, restyles XML widgets exactly like C++ ones.
 */

#pragma once

#include <string_view>
#include <vector>

namespace UI::Xml::ThemeStyles
{
	/** Register every theme style and font with the XML engine. Call after lv_xml_init(). */
	void registerAll();

	/** The names fonts are registered under, e.g. "font_header" */
	std::vector<std::string_view> fontNames();

	/** The names images are registered under, e.g. "symbol_ok" */
	std::vector<std::string_view> imageNames();
} // namespace UI::Xml::ThemeStyles

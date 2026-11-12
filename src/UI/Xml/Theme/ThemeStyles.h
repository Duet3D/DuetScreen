/*
 * ThemeStyles.h
 *
 * Makes the theme's styles and fonts available to XML by name: every member of LvglStyles and ComponentStyles
 * (`<style name="card"/>`) and the four theme fonts (`style_text_font="font_header"`).
 *
 * XML uses the theme's own style objects rather than copies, so a theme switch, which rewrites those objects in
 * place, restyles XML widgets exactly like C++ ones.
 */

#pragma once

namespace UI::Xml::ThemeStyles
{
	/** Register every theme style and font with the XML engine. Call after lv_xml_init(). */
	void registerAll();
} // namespace UI::Xml::ThemeStyles

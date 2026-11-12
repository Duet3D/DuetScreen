/*
 * ThemeFile.h
 *
 * Reads a theme file. A theme is XML, usually in a UI bundle's `themes/` folder (see docs/XML_UI.md):
 *
 *     <theme name="flat" extends="base">
 *         <palette chroma="0"/>                         <!-- see Palette.h -->
 *         <fonts>
 *             <font name="header" size="18" weight="700" style="bold"/>
 *         </fonts>
 *         <consts>
 *             <const name="radius" value="5"/>
 *         </consts>
 *         <styles>
 *             <style name="card" border_width="0"/>    <!-- see Theme.h for how styles are built -->
 *             <style name="folder" copy="btn"/>
 *         </styles>
 *     </theme>
 *
 * This only reads the file; resolving `extends` and building the styles is Theme's job.
 */

#pragma once

#include "Palette.h"
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Themes
{
	/** One font of a theme: header, normal, emphasis or subdued. Unset fields come from the parent theme. */
	struct FontSpec
	{
		std::optional<uint32_t> size;	// Pixel size
		std::optional<uint32_t> weight; // e.g. 400 or 700; 0 (`weight="none"`) for the face's own weight
		std::optional<std::string> style; // normal, bold, italic or "bold italic"
	};

	/** One `<style>` element: a change to one style */
	struct StyleEdit
	{
		std::string name;									   // Style to change
		std::string copy;									   // Replace the style with this one first; empty for none
		std::vector<std::string> merge;						   // Then add these styles' properties, in order
		std::vector<std::pair<std::string, std::string>> props; // Then set these properties (unresolved values)
		int line = 0;										   // Line in the file, for messages
	};

	/** The contents of one theme file */
	struct ThemeFile
	{
		std::string name;			  // Theme name
		std::string extends;		  // Parent theme, or empty
		bool abstract = false;		  // Only a parent for other themes; never shown to users
		std::filesystem::path source; // File it was read from

		std::map<std::string, std::string> palette;	 // `<palette>` attributes (inputs and customisable)
		std::vector<ColorAdjustment> colors;		 // `<color>` adjustments inside `<palette>`
		std::map<std::string, FontSpec> fonts;		 // Fonts by name
		std::vector<std::pair<std::string, std::string>> consts; // Consts in order (name, unresolved value)
		std::vector<StyleEdit> styles;							 // Style changes in order
	};

	/**
	 * Read a theme from `text`
	 * @param source File name used in error messages and kept in the result
	 * @param error Set to a message (with a line number where possible) on failure
	 * @return the theme, or nullopt on failure
	 */
	std::optional<ThemeFile> parseThemeFile(const std::string& text,
											const std::filesystem::path& source,
											std::string& error);
} // namespace UI::Themes

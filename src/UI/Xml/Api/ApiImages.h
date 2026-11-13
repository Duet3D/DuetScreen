/*
 * ApiImages.h
 *
 * Pictures for the UI API reference (ApiMarkdown.h), drawn by the UI itself so they always show what the code does:
 *   - in every built-in theme: the palette, the fonts, a swatch of each style, each widget's example
 *     (WidgetSpec::example, with WidgetSpec::exampleSubjects for what it binds to), a sample of LVGL's own widgets,
 *     and the home screen
 *   - in the active theme: each component that is a view of its own (Component::isView()), with whatever printer
 *     data the Model holds. A view shown by a `<ds_view>` is drawn through a copy of that `<ds_view>`, so it has the
 *     size it is given there; a view that shows in the picture of a view containing it (the status bar in the home
 *     screen's) isn't pictured again, but refers to that picture.
 *
 * A picture that is one colour throughout shows nothing (a closed dialog, a hidden widget, an `<action>`), and a
 * style that changes nothing on its own (a part's or a state's) looks just like the sample box without it; neither is
 * kept, so what is pictured follows from what the code draws rather than from a list.
 *
 * The renderer only draws: for each picture it cuts the display down to it and hands it to the caller, which compares
 * it with the reference image of that name (EXPECT_EQUAL_PICTURE in the tests). It needs a display whose buffer
 * follows its resolution and holds the whole screen, as the tests' display does.
 */

#pragma once

#include "ApiReference.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace UI::Xml::Api
{
	/** Where the pictures go, relative to the reference images (tests/ref_imgs/) */
	inline constexpr const char* IMAGES_DIR = "xml_ui_api";

	/** The pictures of one theme */
	struct ThemePictures
	{
		std::string colours;						// The palette; empty if none
		std::string fonts;							// The fonts; empty if none
		std::string lvglWidgets;					// A sample of LVGL's widgets; empty if none
		std::string home;							// The home screen; empty if none
		std::map<std::string, std::string> styles;	// Style name → its swatch
		std::map<std::string, std::string> widgets; // Widget name → its example
	};

	/** The pictures for the reference, and what each shows; files are relative to the reference images */
	struct Images
	{
		std::string theme;							   // The active theme, which the views are drawn in
		std::map<std::string, ThemePictures> themes;   // Built-in theme name → its pictures
		std::map<std::string, std::string> views;	   // Component name → its own picture
		std::map<std::string, std::string> components; // Component name → the picture it is seen in
		std::vector<std::string> files;				   // Every picture, in the order drawn
	};

	/**
	 * Draw the pictures for `reference` on the active screen, then remove what was drawn and make the active theme
	 * active again
	 * @param settle Called once a view is built, to let Model events (and the data they carry) reach its presenter
	 * @param picture Called with the display cut down to each picture, to compare it with the reference image `file`
	 */
	Images render(const Reference& reference,
				  const std::function<void()>& settle,
				  const std::function<void(const std::string& file)>& picture);
} // namespace UI::Xml::Api

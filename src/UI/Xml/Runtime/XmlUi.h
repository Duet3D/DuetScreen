/*
 * XmlUi.h
 *
 * The runtime-loaded XML UI. Start it once LVGL and the theme are up; views are then XML components (see
 * View/XmlView.h) loaded from bundles (see Bundles.h).
 *
 * How the parts fit together:
 *
 *   Bundles      load XML components and themes from the built-in bundle and user bundles on the SD card
 *   XmlView      shows a component; creates the presenter it names
 *   XmlPresenter C++ behaviour for a view: declares subjects and lists, handles actions
 *   Data sources publish printer data (fans, tools, …) as global subjects and lists
 *   Widgets      DuetScreen's own XML elements (ds_button, ds_slider, ds_list)
 *   Actions      `<action>` elements, routed to presenters or global handlers
 *   Scope        the names a view can bind to while it is built
 */

#pragma once

#include "Bundles.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	class XmlView;

	/** The UI API version this firmware provides. Bump the major version when a name is removed or changes meaning. */
	inline constexpr int UI_API_VERSION_MAJOR = 1;
	inline constexpr int UI_API_VERSION_MINOR = 0;

	/** The built-in bundle (assets/ui/), and user bundles in /boot/ui/ on the screen or var/ui/ in the simulator */
	Paths defaultPaths();

	/**
	 * Start the XML UI: the engine, theme styles, translations, widgets, actions, data sources, then the bundles. The
	 * bundles include the themes, so this also applies the stored theme (see UI/Styles/Theme.h). Presenters must be
	 * registered first (see View/XmlPresenter.h). Call after `Themes::init()`.
	 */
	void init(const Paths& paths = defaultPaths());

	/** Stop the XML UI. Every XmlView must already be destroyed. Call before `lv_deinit()`. */
	void deinit();

	/** @return whether init() has been called without deinit() */
	bool isInitialized();

	/**
	 * Load the bundles again and rebuild every live XmlView (hot reload, or after installing a bundle). The open
	 * screens and selected tabs are kept (see ReloadState.h); anything else a view held, such as an open dialog or
	 * a scroll position, starts afresh.
	 */
	void reload();

	/** Problems found by the last load */
	const std::vector<LoadIssue>& getIssues();

	/** The loaded definition of a component, or nullptr */
	const ComponentInfo* findComponent(std::string_view name);

	/** Rebuild `view` on reload. Called by XmlView. */
	void addView(XmlView* view);

	/** Stop rebuilding `view` on reload. Called by XmlView. */
	void removeView(XmlView* view);
} // namespace UI::Xml

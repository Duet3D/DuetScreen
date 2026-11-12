/*
 * XmlUi.h
 *
 * Runtime-loaded XML UI. Views are XML components loaded from bundles at start-up:
 *
 *   1. the built-in bundle, shipped with the firmware (assets/ui/)
 *   2. user bundles, one per sub-directory of the user UI folder (on the SD card's boot partition on the screen)
 *
 * A component in a later bundle replaces the one of the same name from an earlier bundle, so a user can override
 * e.g. just `fan_card` and every fan list picks it up. A component that fails to load leaves the earlier definition
 * in place and is reported as a load issue (logged and shown as a warning message).
 *
 * Each bundle directory has a manifest.json:
 *
 *     {
 *       "id": "my-dashboard",
 *       "name": "My dashboard",
 *       "version": "1.0.0",
 *       "ui_api": "1.0",
 *       "components": ["components/fan_card.xml", "screens/main.xml"]
 *     }
 *
 * A component entry may use one `*` wildcard in its file name, e.g. "components/" followed by "*.xml".
 *
 * `ui_api` is the major.minor version of the subjects, models, actions and widgets the bundle was written against.
 * A bundle with a different major version is not loaded.
 */

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace UI
{
	class ModalNumberPad;
}

namespace UI::Xml
{
	class XmlView;

	/** The UI API version this firmware provides. Bump the major version when a name is removed or changes meaning. */
	inline constexpr int UI_API_VERSION_MAJOR = 1;
	inline constexpr int UI_API_VERSION_MINOR = 0;

	struct Paths
	{
		/** The built-in bundle directory (contains manifest.json) */
		std::filesystem::path builtin;
		/** Directory of user bundle directories; empty to disable */
		std::filesystem::path user;
	};

	/** assets/ui/ and the SD card's boot partition (/boot/ui/) on the screen; assets/ui/ and var/ui/ in simulation */
	Paths defaultPaths();

	struct LoadIssue
	{
		std::filesystem::path file;
		std::string message;
	};

	struct ComponentInfo
	{
		std::string name;
		std::string presenter;
		std::filesystem::path source;
		std::string bundle;
	};

	/**
	 * Start the XML UI: the engine, theme styles and fonts, translations, widgets, actions, presenters, the object
	 * model publisher, then the bundles. Call after `Themes::init()`.
	 */
	void init(const Paths& paths = defaultPaths());

	/** Tear everything down. Every XmlView must already be destroyed. Call before `lv_deinit()`. */
	void deinit();

	bool isInitialized();

	/**
	 * Load the bundles again and rebuild every live XmlView. Used for hot reload and by the settings to apply a
	 * changed bundle without restarting.
	 */
	void reload();

	/** Problems found by the last load */
	const std::vector<LoadIssue>& getIssues();

	/** The loaded definition of a component, or nullptr */
	const ComponentInfo* findComponent(std::string_view name);

	/** Poll the bundle files every `periodMs` and reload when one changes (simulation builds). 0 stops. */
	void setHotReload(uint32_t periodMs);

	/** Send G-code. Defaults to the Duet connection; tests replace it. */
	void setGcodeSender(std::function<void(std::string_view)> sender);
	void sendGcode(std::string_view gcode);

	/** Where `<ds_slider numberpad="true">` finds the shared number pad (set by HomeView) */
	void setNumberPadProvider(std::function<ModalNumberPad*()> provider);
	ModalNumberPad* getNumberPad();

	/* XmlView bookkeeping, for reload */
	void addView(XmlView* view);
	void removeView(XmlView* view);
} // namespace UI::Xml

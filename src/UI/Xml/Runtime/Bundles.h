/*
 * Bundles.h
 *
 * Loads UI bundles: directories of XML components with a manifest.json.
 *
 *     {
 *       "id": "my-dashboard",
 *       "name": "My dashboard",
 *       "version": "1.0.0",
 *       "ui_api": "1.0",
 *       "components": ["components/fan_card.xml", "screens/main.xml"],
 *       "themes": ["themes/midnight.xml"]
 *     }
 *
 * A component's name is its file name without `.xml`; a theme's is its `name` attribute (see UI/Styles/Theme.h).
 * An entry may use one `*` wildcard in its file name, e.g. "components/" followed by "*.xml". `ui_api` is the major.minor version of the UI API (data, actions, widgets)
 * the bundle was written for; a bundle for a different major version is not loaded.
 *
 * Bundles load in order: the built-in bundle, then each user bundle in name order. A component or theme in a later
 * bundle replaces the one of the same name from an earlier bundle. A component that fails to load (malformed XML, an unknown
 * presenter) leaves the earlier definition in place and is reported as a LoadIssue.
 */

#pragma once

#include "UI/Styles/ThemeFile.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace UI::Xml
{
	/** Where bundles are loaded from */
	struct Paths
	{
		std::filesystem::path builtin; // The built-in bundle directory (contains manifest.json)
		std::filesystem::path user;	   // Directory of user bundle directories; empty for none
	};

	/** A problem found while loading */
	struct LoadIssue
	{
		std::filesystem::path file; // File with the problem
		std::string message;		// What is wrong, e.g. "line 3: mismatched tag"
	};

	/** A loaded component */
	struct ComponentInfo
	{
		std::string name;			  // Component name
		std::string presenter;		  // Presenter named on its root element, or empty
		std::filesystem::path source; // File it was loaded from
		std::string bundle;			  // Id of the bundle it came from
	};

	namespace Bundles
	{
		/** What a load produced */
		struct Result
		{
			std::map<std::string, ComponentInfo, std::less<>> components; // Loaded components by name
			std::vector<Themes::ThemeFile> themes;						  // Themes read, in load order
			std::vector<LoadIssue> issues;								  // Problems found
			std::vector<std::filesystem::path> files; // Every file read (manifests and components), for hot reload
		};

		/** Load the built-in bundle, then the user bundles, registering their components with the XML engine */
		Result loadAll(const Paths& paths);
	} // namespace Bundles
} // namespace UI::Xml

/*
 * NavigationActions.h
 *
 * The global actions that move between screens, for any view (see View/Actions.h): `actions.navigation.open`,
 * `actions.navigation.home` and `actions.navigation.back`. What each does is in its registration, and in the UI API
 * reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::Xml::NavigationActions
{
	/** Register the navigation actions as global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::NavigationActions

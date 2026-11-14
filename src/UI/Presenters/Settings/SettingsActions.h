/*
 * SettingsActions.h
 *
 * Changing a setting, for any view (src/UI/Xml/Data/SettingsSource.h): `actions.settings.set` and
 * `actions.settings.ask`. What each does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::SettingsActions
{
	/** Register the settings actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::SettingsActions

/*
 * DisplaySettings.h
 *
 * The settings of the Display page (settings_display.xml): theme, typeface, icons, rotation, brightness and dimming. Each is declared once, in registerAll(), and published as source.settings.<name>
 * (src/UI/Xml/Data/SettingsSource.h); actions.settings.set changes it.
 */

#pragma once

namespace UI::DisplaySettings
{
	/** Add the settings. Call once, before the XML UI starts. */
	void registerAll();
} // namespace UI::DisplaySettings

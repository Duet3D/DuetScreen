/*
 * DeveloperSettings.h
 *
 * The settings of the Developer page (settings_developer.xml): logging, advanced settings, and build tools. Each is declared once, in registerAll(), and published as source.settings.<name>
 * (src/UI/Xml/Data/SettingsSource.h); actions.settings.set changes it.
 */

#pragma once

namespace UI::DeveloperSettings
{
	/** Add the settings. Call once, before the XML UI starts. */
	void registerAll();
} // namespace UI::DeveloperSettings

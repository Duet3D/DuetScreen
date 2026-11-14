/*
 * GeneralSettings.h
 *
 * The settings of the General page (settings_general.xml): language, USB keyboard layout, and how the screen works the machine. Each is declared once, in registerAll(), and published as source.settings.<name>
 * (src/UI/Xml/Data/SettingsSource.h); actions.settings.set changes it.
 */

#pragma once

namespace UI::GeneralSettings
{
	/** Add the settings. Call once, before the XML UI starts. */
	void registerAll();
} // namespace UI::GeneralSettings

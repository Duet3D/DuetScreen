/*
 * ConnectionSettings.h
 *
 * The settings of the Connection page (settings_connection.xml): how the screen reaches the Duet. Each is declared once, in registerAll(), and published as source.settings.<name>
 * (src/UI/Xml/Data/SettingsSource.h); actions.settings.set changes it.
 */

#pragma once

namespace UI::ConnectionSettings
{
	/** Add the settings. Call once, before the XML UI starts. */
	void registerAll();
} // namespace UI::ConnectionSettings

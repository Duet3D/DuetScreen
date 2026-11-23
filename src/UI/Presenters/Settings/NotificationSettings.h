/*
 * NotificationSettings.h
 *
 * The settings of the Notifications page (settings_notifications.xml): which messages show, and for how long. Each is declared once, in registerAll(), and published as source.settings.<name>
 * (src/UI/Xml/Data/SettingsSource.h); actions.settings.set changes it.
 */

#pragma once

namespace UI::NotificationSettings
{
	/** Add the settings. Call once, before the XML UI starts. */
	void registerAll();
} // namespace UI::NotificationSettings

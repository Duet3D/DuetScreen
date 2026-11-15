/*
 * SimulatorSettings.h
 *
 * The settings of the simulated Duet's page (settings_simulator.xml) that apply to the whole machine: the preset it
 * starts from, its name, its mode and a status to force, and how it behaves as a connection (answering, plugged in,
 * replying late, its temperatures' noise). Each is declared once, in registerAll(), and published as
 * source.settings.simulator_<name> (src/UI/Xml/Data/SettingsSource.h); actions.settings.set changes it. Its parts are
 * in source.simulator (src/UI/Xml/Data/SimulatorSource.h).
 *
 * They are in every build, so the UI API is the same in all; without the simulated Duet (DUET_SIMULATOR) they read 0
 * and do nothing.
 */

#pragma once

namespace UI::SimulatorSettings
{
	/** Add the settings. Call once, before the XML UI starts. */
	void registerAll();
} // namespace UI::SimulatorSettings

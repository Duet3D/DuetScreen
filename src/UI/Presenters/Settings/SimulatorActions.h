/*
 * SimulatorActions.h
 *
 * `actions.simulator.*`, available to every view: change the simulated Duet's parts (source.simulator,
 * src/UI/Xml/Data/SimulatorSource.h) from its page in the settings, and make it show a message box. What each does is
 * in its registration, and in the UI API reference (docs/XML_UI_API.md). In a build without the simulated Duet they do
 * nothing.
 */

#pragma once

namespace UI::SimulatorActions
{
	/** Register the actions as global actions */
	void registerAll();
} // namespace UI::SimulatorActions

/*
 * HeaterActions.h
 *
 * Actions on the tools, beds and chambers, available to every view. They are raised from a row of a list of tools
 * (the global `tools` list, src/UI/Xml/Data/ToolSource.h, or any list of rows derived from its ToolRow), of a tool's
 * `heaters`, or of the global `beds` or `chambers` lists (src/UI/Xml/Data/HeatSource.h):
 *
 *   tools.toggle          select or deselect the tool
 *   heaters.toggle        cycle the heater through active, standby and off
 *   heaters.set_active    set the heater's active temperature: to the edited value (`value_confirmed`, e.g. a
 *                         <ds_bar_marker> dragged), else ask for it with the nearest number pad
 *   heaters.set_standby   the same for the standby temperature
 */

#pragma once

namespace UI::HeaterActions
{
	/** Register the actions as global actions */
	void registerAll();
} // namespace UI::HeaterActions

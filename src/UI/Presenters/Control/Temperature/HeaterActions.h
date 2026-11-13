/*
 * HeaterActions.h
 *
 * Actions on the tools, beds and chambers, available to every view: `tools.toggle`, `heaters.toggle`,
 * `heaters.set_active` and `heaters.set_standby`. They are raised from a row of a list of tools (the global `tools`
 * list, src/UI/Xml/Data/ToolSource.h, or any list of rows derived from its ToolRow), of a tool's `heaters`, or of the
 * global `beds` or `chambers` lists (src/UI/Xml/Data/HeatSource.h). What each does is in its registration, and in the
 * UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::HeaterActions
{
	/** Register the actions as global actions */
	void registerAll();
} // namespace UI::HeaterActions

/*
 * ToolActions.h
 *
 * The global action that selects a tool, for any view: `actions.tool.toggle`, raised from a row of a list of tools
 * (the global `tools` list, src/UI/Xml/Data/ToolSource.h, or any list of rows derived from its ToolRow). What it does is
 * in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::ToolActions
{
	/** Register the tool actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::ToolActions

/*
 * ToolActions.h
 *
 * The global action that selects a tool, for any view: `actions.tool.toggle`, given the tool's number as its `arg`.
 * What it does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::ToolActions
{
	/** Register the tool actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::ToolActions

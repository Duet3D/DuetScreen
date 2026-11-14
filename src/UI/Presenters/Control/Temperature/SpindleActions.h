/*
 * SpindleActions.h
 *
 * The global actions that set and turn a tool's spindle, for any view: `actions.spindle.*`, given the tool's number as
 * their `arg`. They work whatever the machine's mode, so a printer's tool may have a spindle too. Each tool keeps its
 * own speed (tools[].spindleRpm). What they do is in their registration, and in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::SpindleActions
{
	/** Register the spindle actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::SpindleActions

/*
 * FilamentActions.h
 *
 * The global actions that load and unload a tool's filament, for any view: `actions.filament.*`, given the tool's
 * number as their `arg`. Choosing a filament loads it at once, unloading any loaded first, so no view keeps a filament
 * picked but not loaded. What they do is in their registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::FilamentActions
{
	/** Register the filament actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::FilamentActions

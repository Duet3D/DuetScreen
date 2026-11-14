/*
 * ExtruderActions.h
 *
 * Extruding and retracting with the current tool, for any view (src/UI/Xml/Data/ExtruderSource.h):
 * `actions.extruder.extrude` and `actions.extruder.retract`. What each does is in its registration, and in the UI API
 * reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::ExtruderActions
{
	/** Register the extruder actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::ExtruderActions

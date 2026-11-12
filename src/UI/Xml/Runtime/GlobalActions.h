/*
 * GlobalActions.h
 *
 * Actions every view can raise, whatever its presenter (see View/Actions.h):
 *
 *   gcode             send `arg` as G-code, e.g. `<action name="gcode" arg="M106 P{number} S{value}"/>`
 */

#pragma once

namespace UI::Xml::GlobalActions
{
	/** Register the global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::GlobalActions

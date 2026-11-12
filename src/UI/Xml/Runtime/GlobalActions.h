/*
 * GlobalActions.h
 *
 * Actions every view can raise, whatever its presenter (see View/Actions.h):
 *
 *   gcode             send `arg` as G-code, e.g. `<action name="gcode" arg="M106 P{number} S{value}"/>`
 *   nav.open          open the screen registered as `arg`, e.g. `<action name="nav.open" arg="control"/>`
 *   nav.home          close every screen and show the home screens
 *   nav.back          close the top modal, or else go back a screen
 */

#pragma once

namespace UI::Xml::GlobalActions
{
	/** Register the global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::GlobalActions

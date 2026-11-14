/*
 * MoveActions.h
 *
 * Moving the axes, for any view (src/UI/Xml/Data/AxesSource.h): `actions.move.jog`, `.home`, `.set_position` and
 * `.disable_motors`. A jog goes as far and as fast as chosen in source.jog.* (src/UI/Xml/Data/JogSource.h). What each
 * does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::MoveActions
{
	/** Register the move actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::MoveActions

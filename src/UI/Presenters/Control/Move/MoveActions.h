/*
 * MoveActions.h
 *
 * Moving the axes, for any view (src/UI/Xml/Data/AxesSource.h): `actions.move.jog`, `.home`, `.set_position` and
 * `.disable_motors`. A jog goes as far and as fast as chosen in source.jog.* (src/UI/Xml/Data/JogSource.h). What each
 * does is in its registration, and in the UI API reference (docs/XML_UI_API.md). Also the one setting of moving,
 * source.settings.machine_position_mode: whether positions are the workplace's or the machine's (G53).
 */

#pragma once

namespace UI::MoveActions
{
	/** Register the move actions as global actions, and the machine_position_mode setting. Call once, before the XML
	 * UI starts. */
	void registerAll();
} // namespace UI::MoveActions

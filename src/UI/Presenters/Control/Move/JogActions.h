/*
 * JogActions.h
 *
 * Choosing how far and how fast a jog goes, available to every view: `actions.jog.distance`, `actions.jog.feedrate`,
 * `actions.jog.edit_distance` and `actions.jog.edit_feedrate`, raised from a row of the global `jog.distances` or `jog.feedrates`
 * (src/UI/Xml/Data/JogSource.h), so the Move tab and any jog widgets share one choice. What each does is in its
 * registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::JogActions
{
	/** Register the actions as global actions */
	void registerAll();
} // namespace UI::JogActions

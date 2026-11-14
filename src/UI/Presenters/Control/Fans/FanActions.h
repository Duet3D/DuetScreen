/*
 * FanActions.h
 *
 * `actions.fan.set_speed`, available to every view: set a fan's speed, as the fan gauges do when tapped. What it does
 * is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::FanActions
{
	/** Register the actions as global actions */
	void registerAll();
} // namespace UI::FanActions

/*
 * EStopAction.h
 *
 * The global action `estop.dragged`, raised by the E-stop the sidebar shows (a ds_drag_button,
 * assets/ui/components/sidebar/sidebar.xml) and by any other a view places. What it does is in its registration, and
 * in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::EStopAction
{
	/** Register `estop.dragged` as a global action */
	void registerAll();
} // namespace UI::EStopAction

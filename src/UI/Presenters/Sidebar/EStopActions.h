/*
 * EStopActions.h
 *
 * The global action `actions.estop.dragged`, raised by the E-stop the sidebar shows (a ds_drag_button,
 * assets/ui/components/sidebar/sidebar.xml) and by any other a view places. What it does is in its registration, and
 * in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::EStopActions
{
	/** Register `actions.estop.dragged` as a global action */
	void registerAll();
} // namespace UI::EStopActions

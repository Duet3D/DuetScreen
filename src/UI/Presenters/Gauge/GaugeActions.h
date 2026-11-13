/*
 * GaugeActions.h
 *
 * `gauges.edit`, available to every view: change the value a row of the global `gauges` list shows (see
 * UI/Xml/Data/GaugeSource.h), as the gauge widgets do when tapped. What it does is in its registration, and in the UI
 * API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::GaugeActions
{
	/** Register the actions as global actions */
	void registerAll();
} // namespace UI::GaugeActions

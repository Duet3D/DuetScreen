/*
 * HeaterActions.h
 *
 * Actions on the heaters of the tools, beds and chambers, available to every view: `actions.heater.toggle`,
 * `actions.heater.set_active` and `actions.heater.set_standby`. They are raised from a row of a tool's `heaters`
 * (src/UI/Xml/Data/ToolSource.h), or of the global `beds` or `chambers` lists (src/UI/Xml/Data/HeatSource.h). What each does is in its registration, and in the
 * UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include <cstddef>
#include <optional>

namespace UI::HeaterActions
{
	/** Register the actions as global actions */
	void registerAll();

	/** What a temperature is set on */
	enum class Heated
	{
		ToolHeater, // One of a tool's heaters
		Bed,		// A bed
		Chamber,	// A chamber
	};

	/**
	 * Set the active temperature (or the standby, if not `active`) of a tool's heater, a bed or a chamber to `value`,
	 * or ask for one on the shared number pad if there is none
	 * @param number The tool's, bed's or chamber's number
	 * @param slot A tool heater's position among the tool's heaters; a bed's or chamber's slot, shown on the number
	 *        pad
	 */
	void setTemperature(Heated what, size_t number, size_t slot, bool active, std::optional<float> value);
} // namespace UI::HeaterActions

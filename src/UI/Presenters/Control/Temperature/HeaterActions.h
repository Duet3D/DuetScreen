/*
 * HeaterActions.h
 *
 * Actions on the heaters of the tools, beds and chambers, available to every view: `actions.heater.toggle`,
 * `actions.heater.set_active` and `actions.heater.set_standby`. They are raised from a row of an object model list of
 * heaters: a tool's (`om.tools[…].heaters`), the beds' (`om.heat.bedHeaters`) or the chambers'
 * (`om.heat.chamberHeaters`), and tell which from where the row's item is. What each does is in its registration, and in the
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

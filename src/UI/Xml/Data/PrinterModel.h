/*
 * PrinterModel.h
 *
 * Reads the object model (ObjectModel/Tree/Tree.h) the way the heater and tool screens need it: the names DuetScreen
 * gives heaters, beds, chambers and tools when the printer gives none, which heaters are beds and chambers, the
 * filament a tool has loaded, and a heater's target. Shared by the data sources and actions of those screens, so the
 * rules are written once.
 *
 * Hold the model lock (MODEL_LOCK) while calling these and while using the nodes they return.
 */

#pragma once

#include "ObjectModel/Tree/Node.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml::PrinterModel
{
	/** A bed or chamber that has a heater */
	struct HeatedPlace
	{
		size_t number; // The bed or chamber number, as M140/M141 P takes it
		size_t heater; // Its heater's number
	};

	/** The object model */
	const OM::Node& root();

	/** Heater `number` (heat.heaters[number]), or a missing node */
	const OM::Node& heater(size_t number);

	/** Tool `number` (tools[number]), or a missing node */
	const OM::Node& tool(size_t number);

	/** The numbers of the tools there are, in order */
	std::vector<size_t> tools();

	/** The beds that have a heater, in order */
	std::vector<HeatedPlace> beds();

	/** The chambers that have a heater, in order */
	std::vector<HeatedPlace> chambers();

	/** The current tool's number, or nothing if no tool is selected */
	std::optional<size_t> currentTool();

	/** Heater `number`'s name: its sensor's, or e.g. "Heater 2" */
	std::string heaterName(size_t number);

	/** Tool `number`'s name: its own, or e.g. "Tool 0" */
	std::string toolName(size_t number);

	/** Heater `number`'s state key: off, standby, active, fault, tuning or offline ("off" if unknown) */
	std::string heaterState(size_t number);

	/** What heater `number` is heating to: its active target if active, its standby one in standby, else 0 */
	int32_t heaterTarget(size_t number);

	/** The number of the heater tool `number` has at position `slot` of its heaters, or nothing */
	std::optional<size_t> toolHeater(size_t number, size_t slot);

	/** Whether tool `number` takes filament (it has a filament extruder) */
	bool takesFilament(size_t number);

	/**
	 * The names of the tools whose list `list` (e.g. "fans", "extruders") holds `number`, e.g. "Revo, Tool 2": a tool's
	 * name, else "Tool <number>"; empty if none does
	 */
	std::string toolsUsing(std::string_view list, size_t number);

	/** The filament loaded in tool `number`'s filament extruder; empty if none */
	std::string_view filament(size_t number);

	/** The axis whose letter is `letter` (move.axes[…]), or a missing node */
	const OM::Node& axis(char letter);

	/** The number of the axis whose letter is `letter`, or nothing */
	std::optional<size_t> axisNumber(char letter);

	/** The current tool's offset along the axis whose letter is `letter`; 0 with no current tool or no such axis */
	float toolOffset(char letter);

	/** The axes there are and the user sees (not hidden), in order, by number */
	std::vector<size_t> visibleAxes();

	/** `letter` as G-code writes an axis: as it is, or with a ' before a lower case letter */
	std::string axisGcode(char letter);

	/** Whether the printer refuses to move an axis that isn't homed */
	bool noMovesBeforeHoming();
} // namespace UI::Xml::PrinterModel

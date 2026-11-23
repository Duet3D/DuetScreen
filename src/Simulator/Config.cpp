/*
 * Config.cpp
 */

#include "Config.h"
#include <algorithm>
#include <array>

namespace Sim
{
	namespace
	{
		/** X, Y and Z of a bed `size` mm square and `height` mm tall */
		std::vector<Axis> cartesian(float size, float height)
		{
			return {{.letter = "X", .min = 0.0f, .max = size},
					{.letter = "Y", .min = 0.0f, .max = size},
					{.letter = "Z", .min = 0.0f, .max = height}};
		}

		/** A heated bed */
		Heater bed()
		{
			return {.name = "Bed", .role = HeaterRole::bed, .max = 120.0f, .heatingRate = 2.0f, .coolingRate = 0.8f};
		}

		/** A hotend called `name` */
		Heater hotend(std::string name)
		{
			return {.name = std::move(name),
					.role = HeaterRole::tool,
					.max = 285.0f,
					.heatingRate = 10.0f,
					.coolingRate = 4.0f};
		}

		/** Some jobs to print */
		std::vector<JobFile> jobs()
		{
			return {{.name = "benchy.gcode", .printTime = 3600, .layers = 240},
					{.name = "calibration cube.gcode", .printTime = 900, .layers = 100},
					{.name = "first layer test.gcode", .printTime = 120, .layers = 1, .thumbnail = false}};
		}

		/** One hotend, one tool, a part cooling fan and a hotend fan */
		Config singleTool()
		{
			return {.preset = "single_tool",
					.name = "Simulated FFF",
					.mode = MachineMode::fff,
					.axes = cartesian(300.0f, 300.0f),
					.heaters = {bed(), hotend("Hotend")},
					.extruders = {{.filament = "PLA"}},
					.tools = {{.name = "Hotend", .heaters = {1}, .extruders = {0}, .fans = {0}}},
					.fans = {{.name = "Part cooling"}, {.name = "Hotend", .tacho = true, .thermostatic = true}},
					.sensors = {{.kind = SensorKind::filamentMonitor, .name = "Filament", .value = 1.0f},
								{.kind = SensorKind::probe, .name = "Probe", .value = 0.0f}},
					.files = jobs()};
		}

		/** Two independent carriages (X and U), each with its own hotend */
		Config idex()
		{
			Config config{.preset = "idex",
						  .name = "Simulated IDEX",
						  .mode = MachineMode::fff,
						  .axes = cartesian(330.0f, 300.0f),
						  .heaters = {bed(), hotend("Left"), hotend("Right")},
						  .extruders = {{.filament = "PLA"}, {.filament = "PETG"}},
						  .tools = {{.name = "Left", .heaters = {1}, .extruders = {0}, .fans = {0}},
									{.name = "Right", .heaters = {2}, .extruders = {1}, .fans = {1}}},
						  .fans = {{.name = "Left cooling"}, {.name = "Right cooling"}},
						  .sensors = {{.kind = SensorKind::filamentMonitor, .name = "Left filament", .value = 1.0f},
									  {.kind = SensorKind::filamentMonitor, .name = "Right filament", .value = 1.0f}},
						  .files = jobs()};
			config.axes.push_back({.letter = "U", .min = 0.0f, .max = 330.0f});
			return config;
		}

		/** Four tools picked up by a coupler (C), a heated chamber */
		Config toolChanger()
		{
			Config config{.preset = "tool_changer",
						  .name = "Simulated tool changer",
						  .mode = MachineMode::fff,
						  .axes = cartesian(300.0f, 300.0f),
						  .heaters = {bed(),
									  hotend("T0"),
									  hotend("T1"),
									  hotend("T2"),
									  hotend("T3"),
									  {.name = "Chamber",
									   .role = HeaterRole::chamber,
									   .max = 70.0f,
									   .heatingRate = 1.0f,
									   .coolingRate = 0.3f}},
						  .extruders = {{.filament = "PLA"}, {.filament = "PETG"}, {.filament = "TPU"}, {}},
						  .fans = {{.name = "T0 cooling"},
								   {.name = "T1 cooling"},
								   {.name = "T2 cooling"},
								   {.name = "T3 cooling"},
								   {.name = "Chamber", .tacho = true}},
						  .sensors = {{.kind = SensorKind::temperature, .name = "Electronics", .value = 35.0f},
									  {.kind = SensorKind::probe, .name = "Probe", .value = 0.0f}},
						  .files = jobs()};
			for (int tool = 0; tool < 4; tool++)
			{
				config.tools.push_back(
					{.name = "T" + std::to_string(tool), .heaters = {tool + 1}, .extruders = {tool}, .fans = {tool}});
			}
			config.axes.push_back({.letter = "C", .min = 0.0f, .max = 360.0f});
			return config;
		}

		/** A mill: a spindle tool, a rotary A axis */
		Config cnc()
		{
			Config config{.preset = "cnc",
						  .name = "Simulated CNC",
						  .mode = MachineMode::cnc,
						  .axes = cartesian(600.0f, 120.0f),
						  .tools = {{.name = "Spindle", .spindle = 0}},
						  .sensors = {{.kind = SensorKind::probe, .name = "Touch plate", .value = 0.0f},
									  {.kind = SensorKind::input, .name = "Door", .value = 0.0f}},
						  .spindles = {{.max = 24000, .canReverse = true}},
						  .files = {{.name = "pocket.gcode", .printTime = 1800, .layers = 1},
									{.name = "engrave.nc", .printTime = 600, .layers = 1}}};
			config.axes.push_back({.letter = "A", .min = 0.0f, .max = 360.0f});
			return config;
		}

		/** A laser cutter */
		Config laser()
		{
			return {.preset = "laser",
					.name = "Simulated laser",
					.mode = MachineMode::laser,
					.axes = cartesian(400.0f, 50.0f),
					.tools = {{.name = "Laser"}},
					.fans = {{.name = "Air assist"}},
					.files = {{.name = "cut.gcode", .printTime = 900, .layers = 1}}};
		}
	} // namespace

	const std::vector<Preset>& presets()
	{
		static const std::vector<Preset> all = {
			{.key = "single_tool", .make = singleTool},
			{.key = "idex", .make = idex},
			{.key = "tool_changer", .make = toolChanger},
			{.key = "cnc", .make = cnc},
			{.key = "laser", .make = laser},
		};
		return all;
	}

	Config defaultConfig()
	{
		return presets().front().make();
	}

	std::span<const char* const> axisLetters()
	{
		static constexpr std::array letters = {"X", "Y", "Z", "U", "V", "W", "A", "B", "C", "D"};
		return letters;
	}

	std::string nextAxisLetter(const std::vector<Axis>& axes)
	{
		for (const char* letter : axisLetters())
		{
			if (std::none_of(axes.begin(), axes.end(), [letter](const Axis& axis) { return axis.letter == letter; }))
			{
				return letter;
			}
		}
		return "";
	}

	std::string_view modeObjectModelName(MachineMode mode)
	{
		switch (mode)
		{
		case MachineMode::cnc:
			return "CNC";
		case MachineMode::laser:
			return "Laser";
		case MachineMode::fff:
		default:
			return "FFF";
		}
	}
} // namespace Sim

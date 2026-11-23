/*
 * Config.h
 *
 * What the simulated Duet is: its axes, heaters, extruders, tools, fans, sensors, spindles and the jobs on its SD
 * card. A
 * configuration starts from a preset (presets()) and is edited item by item on the screen (Settings > Connection >
 * Simulated Duet), then kept with the screen's settings. Machine.h builds the running machine from it.
 *
 * Built only with the DUET_SIMULATOR build option (development builds and the unit tests).
 */

#pragma once

#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Sim
{
	/** The machine's mode (state.machineMode) */
	enum class MachineMode
	{
		fff,
		cnc,
		laser,
	};

	/** What a heater heats, which decides where the UI shows it */
	enum class HeaterRole
	{
		tool,	 // A tool's heater (named in a tool's heaters)
		bed,	 // In heat.bedHeaters
		chamber, // In heat.chamberHeaters
		other,	 // Anything else, e.g. a dryer
	};

	/** What kind of sensor a sensor is, which decides where it is reported */
	enum class SensorKind
	{
		temperature,	 // sensors.analog, after the heaters' own; value °C
		filamentMonitor, // sensors.filamentMonitors; value 1 if filament is present, else 0
		probe,			 // sensors.probes; value its reading, 0-1000
		input,			 // sensors.gpIn; value 0 or 1
	};

	/** An axis */
	struct Axis
	{
		std::string letter = "X"; // X, Y, Z, U, V, W, A, B, C, …
		float min = 0.0f;		  // Lowest position, mm (or ° for a rotary axis)
		float max = 300.0f;		  // Highest position
	};

	/** A heater, with a sensor of its own (sensor number = heater number) */
	struct Heater
	{
		std::string name{};				// Its sensor's name, e.g. "Hotend"
		HeaterRole role = HeaterRole::tool; // What it heats
		float max = 285.0f;				// Highest temperature it may be set to, °C
		float heatingRate = 8.0f;		// How fast it heats at full power, °C/s
		float coolingRate = 3.0f;		// How fast it cools at most, °C/s
		bool fault = false;				// In fault: it reads but can't heat
	};

	/** An extruder */
	struct Extruder
	{
		std::string filament{}; // The filament loaded when the machine starts, e.g. "PLA"; "" for none
	};

	/** A tool */
	struct Tool
	{
		std::string name{};			 // E.g. "Left"
		std::vector<int> heaters{};	 // Its heaters' numbers
		std::vector<int> extruders{}; // Its extruders' numbers; the first is the one its filament is loaded into
		std::vector<int> fans{};	 // Its fans' numbers
		int spindle = -1;			 // Its spindle's number, or -1
	};

	/** A fan */
	struct Fan
	{
		std::string name{};		   // E.g. "Part cooling"
		bool tacho = false;		   // Reports its rpm
		bool thermostatic = false; // Turns itself on while the first tool heater is above 45 °C
	};

	/** A sensor that isn't a heater's */
	struct Sensor
	{
		SensorKind kind = SensorKind::temperature; // What it is
		std::string name{};						   // E.g. "Enclosure"
		float value = 25.0f;					   // Its reading (see SensorKind)
	};

	/** A spindle */
	struct Spindle
	{
		int max = 24000;		 // Highest speed, rpm
		bool canReverse = true;	 // Turns both ways (M4)
	};

	/** A job on the SD card, in 0:/gcodes */
	struct JobFile
	{
		std::string name = "job.gcode"; // File name
		int printTime = 600;			// How long it takes at 100% speed, s
		int layers = 50;				// How many layers it has
		bool thumbnail = true;			// Has thumbnails, as a slicer embeds them
	};

	/** The whole machine */
	struct Config
	{
		std::string preset{};				 // The preset it started from (Preset::key)
		std::string name = "Simulated Duet"; // network.name, shown as the machine's name
		MachineMode mode = MachineMode::fff; // state.machineMode
		std::vector<Axis> axes{};				 // In order: the first three are X, Y, Z
		std::vector<Heater> heaters{};		 // By number
		std::vector<Extruder> extruders{};	 // By number
		std::vector<Tool> tools{};			 // By number
		std::vector<Fan> fans{};				 // By number
		std::vector<Sensor> sensors{};		 // Besides the heaters' sensors
		std::vector<Spindle> spindles{};		 // By number
		std::vector<JobFile> files{};			 // Jobs that can be printed
	};

	/** A machine to start from */
	struct Preset
	{
		std::string_view key;		// Its translation key, after `simulator.presets.`
		Config (*make)();			// Makes it
	};

	/** The presets there are; the first is the default */
	const std::vector<Preset>& presets();

	/** The default configuration: the first preset */
	Config defaultConfig();

	/** The letters an axis may have, in the order new axes take them: X Y Z U V W A B C D */
	std::span<const char* const> axisLetters();

	/** The letter for a new axis after `axes`: the first of axisLetters() not used, or "" if every one is */
	std::string nextAxisLetter(const std::vector<Axis>& axes);

	/** state.machineMode for `mode`: "FFF", "CNC" or "Laser" */
	std::string_view modeObjectModelName(MachineMode mode);

	NLOHMANN_JSON_SERIALIZE_ENUM(MachineMode,
								 {{MachineMode::fff, "fff"}, {MachineMode::cnc, "cnc"}, {MachineMode::laser, "laser"}})
	NLOHMANN_JSON_SERIALIZE_ENUM(HeaterRole,
								 {{HeaterRole::tool, "tool"},
								  {HeaterRole::bed, "bed"},
								  {HeaterRole::chamber, "chamber"},
								  {HeaterRole::other, "other"}})
	NLOHMANN_JSON_SERIALIZE_ENUM(SensorKind,
								 {{SensorKind::temperature, "temperature"},
								  {SensorKind::filamentMonitor, "filament_monitor"},
								  {SensorKind::probe, "probe"},
								  {SensorKind::input, "input"}})
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Axis, letter, min, max)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Heater, name, role, max, heatingRate, coolingRate, fault)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Extruder, filament)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Tool, name, heaters, extruders, fans, spindle)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Fan, name, tacho, thermostatic)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Sensor, kind, name, value)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Spindle, max, canReverse)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(JobFile, name, printTime, layers, thumbnail)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
		Config, preset, name, mode, axes, heaters, extruders, tools, fans, sensors, spindles, files)
} // namespace Sim

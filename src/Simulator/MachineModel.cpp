/*
 * MachineModel.cpp
 *
 * The simulated machine as a Duet reports it: its object model, shaped as RepRapFirmware 3.6 shapes it (see the
 * captured models in tests/object_model/), and its replies to M409, M20, M36 and M36.2.
 */

#include "Configuration.h"
#include "Machine.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fmt/format.h>

namespace Sim
{
	namespace
	{
		/** When the simulated machine's clock starts, seconds since 1970 (2026-11-15T12:00:00) */
		constexpr std::time_t CLOCK_START = 1794830400;

		/** The input channels a Duet has, in order */
		constexpr const char* INPUTS[] = {"HTTP",
										  "Telnet",
										  "File",
										  "USB",
										  "Aux",
										  "Trigger",
										  "Queue",
										  "LCD",
										  "SBC",
										  "Daemon",
										  "Aux2",
										  "Autopause",
										  "File2",
										  "Queue2"};

		/** Spindles a Duet reports, configured or not */
		constexpr size_t SPINDLE_SLOTS = 4;

		/** Entries in heat.bedHeaters and heat.chamberHeaters */
		constexpr size_t BED_SLOTS = 12;
		constexpr size_t CHAMBER_SLOTS = 8;

		/** `model` without the null fields of its objects, as a Duet replies without the n flag */
		Json withoutNulls(const Json& model)
		{
			if (model.is_object())
			{
				Json result = Json::object();
				for (const auto& [key, value] : model.items())
				{
					if (!value.is_null())
					{
						result[key] = withoutNulls(value);
					}
				}
				return result;
			}
			if (model.is_array())
			{
				Json result = Json::array();
				for (const auto& value : model)
				{
					result.push_back(value.is_object() ? withoutNulls(value) : value);
				}
				return result;
			}
			return model;
		}
	} // namespace

	double Machine::rounded(double value, int decimals)
	{
		const double scale = std::pow(10.0, decimals);
		return std::round(value * scale) / scale;
	}

	const Json& Machine::liveFields()
	{
		static const Json fields = Json::parse(R"({
			"boards": {"freeRam": true, "mcuTemp": {"current": true}, "v12": {"current": true}, "vIn": {"current": true}},
			"fans": {"actualValue": true, "requestedValue": true, "rpm": true},
			"heat": {"heaters": {"active": true, "avgPwm": true, "current": true, "standby": true, "state": true}},
			"inputs": {"active": true, "feedRate": true, "inMacro": true, "lineNumber": true, "state": true},
			"job": {"build": {"currentObject": true}, "duration": true, "filePosition": true, "layer": true,
					"layerTime": true, "pauseDuration": true, "rawExtrusion": true, "timesLeft": true,
					"warmUpDuration": true},
			"move": {"axes": {"machinePosition": true, "userPosition": true}, "currentMove": true,
					 "extruders": {"position": true, "rawPosition": true}, "virtualEPos": true},
			"sensors": {"analog": {"lastReading": true}, "endstops": {"triggered": true},
						"filamentMonitors": {"status": true}, "gpIn": {"value": true}, "probes": {"value": true}},
			"spindles": {"current": true, "state": true},
			"state": {"currentTool": true, "msUpTime": true, "status": true, "time": true, "upTime": true},
			"tools": {"active": true, "isRetracted": true, "standby": true, "state": true}
		})");
		return fields;
	}

	Json Machine::keep(const Json& model, const Json& mask)
	{
		if (mask.is_boolean())
		{
			return model;
		}
		if (model.is_array())
		{
			Json result = Json::array();
			for (const auto& value : model)
			{
				result.push_back(value.is_null() ? value : keep(value, mask));
			}
			return result;
		}
		if (!model.is_object())
		{
			return model;
		}
		Json result = Json::object();
		for (const auto& [key, value] : model.items())
		{
			if (mask.contains(key))
			{
				result[key] = keep(value, mask[key]);
			}
		}
		return result;
	}

	Json Machine::drop(const Json& model, const Json& mask)
	{
		if (model.is_array())
		{
			Json result = Json::array();
			for (const auto& value : model)
			{
				result.push_back(value.is_null() ? value : drop(value, mask));
			}
			return result;
		}
		if (!model.is_object() || !mask.is_object())
		{
			return model;
		}
		Json result = Json::object();
		for (const auto& [key, value] : model.items())
		{
			if (!mask.contains(key))
			{
				result[key] = value;
			}
			else if (!mask[key].is_boolean())
			{
				result[key] = drop(value, mask[key]);
			}
		}
		return result;
	}

	void Machine::updateSeqs(const Json& model)
	{
		const Json& live = liveFields();
		for (const auto& [key, value] : model.items())
		{
			if (key == "seqs")
			{
				continue;
			}
			std::string digest = (live.contains(key) ? drop(value, live[key]) : value).dump();
			auto& last = m_digests[key];
			if (digest != last)
			{
				last = std::move(digest);
				m_seqs[key]++;
			}
		}
	}

	Json Machine::heaterModel(size_t heater) const
	{
		const HeaterState& state = m_heaters[heater];
		const Heater& config = m_config.heaters[heater];
		return Json{{"active", rounded(state.active, 1)},
					{"avgPwm", rounded(state.pwm, 3)},
					{"current", rounded(state.reading)},
					{"max", config.max},
					{"maxBadReadings", 3},
					{"maxHeatingFaultTime", 5.0},
					{"maxTempExcursion", 15.0},
					{"min", -273.1},
					{"model",
					 Json{{"coolingExp", 1.4},
						  {"coolingRate", 0.56},
						  {"deadTime", 5.5},
						  {"enabled", true},
						  {"heatingRate", config.heatingRate},
						  {"maxPwm", 1.0}}},
					{"monitors", Json::array({Json{{"action", 0}, {"condition", "tooHigh"}, {"limit", config.max}}})},
					{"sensor", heater},
					{"standby", rounded(state.standby, 1)},
					{"state", state.state}};
	}

	Json Machine::jobModel() const
	{
		Json fileInfo{{"customInfo", Json::object()},
					  {"filament", Json::array()},
					  {"fileName", nullptr},
					  {"generatedBy", nullptr},
					  {"height", 0},
					  {"lastModified", nullptr},
					  {"layerHeight", 0},
					  {"numLayers", 0},
					  {"printTime", nullptr},
					  {"simulatedTime", nullptr},
					  {"size", 0},
					  {"thumbnails", Json::array()}};
		const Json lastFileName = m_job.lastFileName.empty() ? Json(nullptr) : Json(m_job.lastFileName);
		const Json lastDuration = m_job.lastFileName.empty() ? Json(nullptr) : Json(std::lround(m_job.lastDuration));
		if (!m_job.running)
		{
			return Json{{"build", nullptr},
						{"duration", nullptr},
						{"file", fileInfo},
						{"filePosition", 0},
						{"lastDuration", lastDuration},
						{"lastFileName", lastFileName},
						{"lastWarmUpDuration", nullptr},
						{"layer", nullptr},
						{"layerTime", nullptr},
						{"pauseDuration", nullptr},
						{"rawExtrusion", nullptr},
						{"timesLeft",
						 Json{{"filament", nullptr}, {"file", nullptr}, {"slicer", nullptr}, {"toPause", nullptr}}},
						{"warmUpDuration", nullptr}};
		}

		const JobFile& file = m_config.files[m_job.file];
		const double filament = static_cast<double>(file.printTime) * 1.5;
		fileInfo["filament"] = Json::array({rounded(filament, 1)});
		fileInfo["fileName"] = "0:/gcodes/" + file.name;
		fileInfo["generatedBy"] = "DuetScreen simulator";
		fileInfo["height"] = rounded(jobHeight(file));
		fileInfo["lastModified"] = fileDate(file);
		fileInfo["layerHeight"] = 0.2;
		fileInfo["numLayers"] = file.layers;
		fileInfo["printTime"] = file.printTime;
		fileInfo["size"] = fileSize(file);

		// Three objects side by side in the middle of the bed, for object cancelling
		Json objects = Json::array();
		const float xMin = m_config.axes.size() > 0 ? m_config.axes[0].min : 0.0f;
		const float xMax = m_config.axes.size() > 0 ? m_config.axes[0].max : 300.0f;
		const float yMin = m_config.axes.size() > 1 ? m_config.axes[1].min : 0.0f;
		const float yMax = m_config.axes.size() > 1 ? m_config.axes[1].max : 300.0f;
		const float width = (xMax - xMin) / 8.0f;
		const float yMiddle = (yMin + yMax) / 2.0f;
		for (size_t i = 0; i < m_job.cancelled.size(); i++)
		{
			const float x = xMin + (xMax - xMin) * (0.25f + 0.2f * static_cast<float>(i));
			objects.push_back(
				Json{{"cancelled", static_cast<bool>(m_job.cancelled[i])},
					 {"name", fmt::format("Part {:d}", i + 1)},
					 {"x", Json::array({std::lround(x), std::lround(x + width)})},
					 {"y", Json::array({std::lround(yMiddle - width / 2), std::lround(yMiddle + width / 2)})}});
		}
		const double layers = std::max(1, file.layers);
		const size_t current = static_cast<size_t>(m_job.progress * layers * 3.0) % std::max<size_t>(1, objects.size());
		const bool printing = !m_job.warming;
		const double remaining =
			(1.0 - m_job.progress) * file.printTime / static_cast<double>(std::max(0.01f, m_speedFactor));

		return Json{
			{"build",
			 Json{{"currentObject", printing && !m_job.cancelled[current] ? static_cast<int>(current) : -1},
				  {"m486Names", true},
				  {"m486Numbers", true},
				  {"objects", objects}}},
			{"duration", std::lround(m_job.duration)},
			{"file", fileInfo},
			{"filePosition", std::llround(m_job.progress * static_cast<double>(fileSize(file)))},
			{"lastDuration", lastDuration},
			{"lastFileName", lastFileName},
			{"lastWarmUpDuration", nullptr},
			{"layer",
			 printing ? Json(std::min(file.layers, static_cast<int>(m_job.progress * layers) + 1)) : Json(nullptr)},
			{"layerTime", printing ? Json(rounded(file.printTime / layers, 1)) : Json(nullptr)},
			{"pauseDuration", std::lround(m_job.pauseDuration)},
			{"rawExtrusion", rounded(m_job.progress * filament, 1)},
			{"timesLeft",
			 Json{{"filament", printing ? Json(std::lround(remaining)) : Json(nullptr)},
				  {"file", printing ? Json(std::lround(remaining)) : Json(nullptr)},
				  {"slicer", printing ? Json(std::lround(remaining)) : Json(nullptr)},
				  {"toPause", nullptr}}},
			{"warmUpDuration", std::lround(m_job.warmUp)}};
	}

	Json Machine::objectModel() const
	{
		// boards
		const Json boards = Json::array({Json{
			{"canAddress", 0},
			{"drivers", Json::array()},
			{"firmwareDate", "2026-11-15"},
			{"firmwareFileName", "Duet3Firmware_MB6HC.bin"},
			{"firmwareName", "RepRapFirmware for Duet 3 MB6HC"},
			{"firmwareVersion", "3.6.0"},
			{"freeRam", 75000},
			{"maxHeaters", MAX_HEATERS},
			{"maxMotors", 6},
			{"mcuTemp", Json{{"current", rounded(40.0 + std::sin(m_upTime / 30.0), 1)}, {"max", 42.0}, {"min", 38.0}}},
			{"name", "Duet 3 MB6HC (simulated)"},
			{"shortName", "MB6HC"},
			{"supportsDirectDisplay", false},
			{"uniqueId", "SIMUL-ATEDD-UETSC-REEN0-00000-00001"},
			{"v12", Json{{"current", 12.1}, {"max", 12.2}, {"min", 12.0}}},
			{"vIn", Json{{"current", 24.0}, {"max", 24.1}, {"min", 23.9}}}}});

		// fans
		Json fans = Json::array();
		for (size_t i = 0; i < m_fans.size(); i++)
		{
			const Fan& fan = m_config.fans[i];
			// As RepRapFirmware reports one: a fan the firmware runs (thermostatic) has the temperatures it runs
			// between; one set by hand only an empty list of sensors
			Json thermostatic = Json{{"sensors", Json::array()}};
			if (fan.thermostatic)
			{
				Json sensors = Json::array();
				if (!m_config.tools.empty() && !m_config.tools.front().heaters.empty())
				{
					sensors.push_back(m_config.tools.front().heaters.front());
				}
				thermostatic = Json{
					{"highTemperature", THERMOSTATIC_ON}, {"lowTemperature", THERMOSTATIC_ON}, {"sensors", sensors}};
			}
			fans.push_back(Json{{"actualValue", rounded(m_fans[i].actual)},
								{"blip", 0.1},
								{"frequency", 250},
								{"max", 1.0},
								{"min", 0.0},
								{"name", fan.name},
								{"requestedValue", rounded(m_fans[i].requested)},
								{"rpm", fan.tacho ? std::lround(m_fans[i].actual * 6000.0f) : -1},
								{"tachoPpr", 2.0},
								{"thermostatic", thermostatic}});
		}

		// heat
		Json heaters = Json::array();
		Json bedHeaters = Json::array();
		Json chamberHeaters = Json::array();
		for (size_t h = 0; h < m_heaters.size(); h++)
		{
			heaters.push_back(heaterModel(h));
			if (m_config.heaters[h].role == HeaterRole::bed)
			{
				bedHeaters.push_back(h);
			}
			else if (m_config.heaters[h].role == HeaterRole::chamber)
			{
				chamberHeaters.push_back(h);
			}
		}
		while (bedHeaters.size() < BED_SLOTS)
		{
			bedHeaters.push_back(-1);
		}
		while (chamberHeaters.size() < CHAMBER_SLOTS)
		{
			chamberHeaters.push_back(-1);
		}
		const Json heat{{"bedHeaters", bedHeaters},
						{"chamberHeaters", chamberHeaters},
						{"coldExtrudeTemperature", 160.0},
						{"coldRetractTemperature", 90.0},
						{"heaters", heaters}};

		// inputs
		Json inputs = Json::array();
		for (const char* name : INPUTS)
		{
			inputs.push_back(Json{{"active", true},
								  {"axesRelative", m_relative},
								  {"compatibility", "RepRapFirmware"},
								  {"distanceUnit", "mm"},
								  {"drivesRelative", m_relativeExtrusion},
								  {"feedRate", rounded(m_feedRate * 60.0f, 1)},
								  {"inMacro", false},
								  {"lineNumber", 0},
								  {"motionSystem", 0},
								  {"name", name},
								  {"state", "idle"},
								  {"volumetric", false}});
		}

		// move
		Json axes = Json::array();
		for (size_t i = 0; i < m_axes.size(); i++)
		{
			const AxisState& axis = m_axes[i];
			const Axis& config = m_config.axes[i];
			const float offset = m_workplace >= 0 ? axis.workplaceOffsets[static_cast<size_t>(m_workplace)] : 0.0f;
			Json offsets = Json::array();
			for (const float value : axis.workplaceOffsets)
			{
				offsets.push_back(rounded(value, 3));
			}
			axes.push_back(Json{{"acceleration", 1000.0},
								{"babystep", rounded(axis.babystep, 3)},
								{"backlash", 0},
								{"current", 1000},
								{"drivers", Json::array({fmt::format("0.{:d}", i)})},
								{"homed", axis.homed},
								{"jerk", 600.0},
								{"letter", config.letter},
								{"machinePosition", rounded(axis.position, 3)},
								{"max", config.max},
								{"maxProbed", false},
								{"microstepping", Json{{"interpolated", true}, {"value", 16}}},
								{"min", config.min},
								{"minProbed", false},
								{"percentCurrent", 100},
								{"speed", 6000.0},
								{"stepsPerMm", 80.0},
								{"userPosition", rounded(axis.position - offset, 3)},
								{"visible", true},
								{"workplaceOffsets", offsets}});
		}
		Json extruders = Json::array();
		for (size_t e = 0; e < m_extruders.size(); e++)
		{
			const ExtruderState& extruder = m_extruders[e];
			extruders.push_back(Json{{"acceleration", 2500.0},
									 {"driver", fmt::format("0.{:d}", m_axes.size() + e)},
									 {"factor", rounded(extruder.factor, 3)},
									 {"filament", extruder.filament},
									 {"filamentDiameter", 1.75},
									 {"position", rounded(extruder.position, 1)},
									 {"pressureAdvance", 0},
									 {"rawPosition", rounded(extruder.position, 1)},
									 {"speed", 3000.0},
									 {"stepsPerMm", 420.0}});
		}
		const bool moving = std::any_of(
			m_axes.begin(), m_axes.end(), [](const AxisState& axis) { return axis.position != axis.target; });
		const float speed =
			moving || (m_job.running && !m_job.warming && !m_transition) ? m_feedRate * m_speedFactor : 0.0f;
		const Json move{{"axes", axes},
						{"backlashFactor", 10},
						{"compensation", compensationModel()},
						{"currentMove",
						 Json{{"acceleration", 0},
							  {"deceleration", 0},
							  {"extrusionRate", 0},
							  {"laserPwm", nullptr},
							  {"requestedSpeed", rounded(speed, 1)},
							  {"topSpeed", rounded(speed, 1)}}},
						{"extruders", extruders},
						{"idle", Json{{"factor", 0.3}, {"timeout", 30.0}}},
						{"kinematics", Json{{"name", "cartesian"}}},
						{"limitAxes", true},
						{"noMovesBeforeHoming", true},
						{"printingAcceleration", 10000.0},
						{"speedFactor", rounded(m_speedFactor)},
						{"travelAcceleration", 10000.0},
						{"virtualEPos", 0},
						{"workplaceNumber", std::max(0, m_workplace)}};

		// sensors: each heater's own, then the others
		Json analog = Json::array();
		for (size_t h = 0; h < m_heaters.size(); h++)
		{
			analog.push_back(Json{{"lastReading", rounded(m_heaters[h].reading)},
								  {"name", m_config.heaters[h].name},
								  {"port", fmt::format("temp{:d}", h)},
								  {"state", "ok"},
								  {"type", "thermistor"}});
		}
		Json filamentMonitors = Json::array();
		Json gpIn = Json::array();
		Json probes = Json::array();
		std::normal_distribution<float> noise(0.0f, 1.0f);
		for (const Sensor& sensor : m_config.sensors)
		{
			switch (sensor.kind)
			{
			case SensorKind::temperature:
				analog.push_back(Json{{"lastReading", rounded(sensor.value)},
									  {"name", sensor.name},
									  {"port", fmt::format("temp{:d}", analog.size())},
									  {"state", "ok"},
									  {"type", "thermistor"}});
				break;
			case SensorKind::filamentMonitor:
				filamentMonitors.push_back(Json{{"enableMode", 1},
												{"enabled", true},
												{"status", sensor.value >= 0.5f ? "ok" : "noFilament"},
												{"type", "simple"}});
				break;
			case SensorKind::probe:
				probes.push_back(Json{{"deployedByUser", false},
									  {"lastStopHeight", 0},
									  {"threshold", 500},
									  {"triggerHeight", 0.7},
									  {"type", 8},
									  {"value", Json::array({std::lround(sensor.value)})}});
				break;
			case SensorKind::input:
				gpIn.push_back(Json{{"value", sensor.value >= 0.5f ? 1 : 0}});
				break;
			}
		}
		Json endstops = Json::array();
		for (const AxisState& axis : m_axes)
		{
			endstops.push_back(
				Json{{"highEnd", false},
					 {"triggered", axis.homed && axis.position <= m_config.axes[&axis - m_axes.data()].min},
					 {"type", "inputPin"}});
		}
		const Json sensors{{"analog", analog},
						   {"endstops", endstops},
						   {"filamentMonitors", filamentMonitors},
						   {"gpIn", gpIn},
						   {"probes", probes}};

		// spindles
		Json spindles = Json::array();
		for (size_t s = 0; s < m_spindles.size(); s++)
		{
			spindles.push_back(Json{{"active", m_spindles[s].active},
									{"canReverse", m_config.spindles[s].canReverse},
									{"current", m_spindles[s].current},
									{"frequency", 0},
									{"idlePwm", 0},
									{"max", m_config.spindles[s].max},
									{"maxPwm", 1.0},
									{"min", 0},
									{"minPwm", 0},
									{"state", m_spindles[s].state},
									{"type", "enaDir"}});
		}
		while (spindles.size() < SPINDLE_SLOTS)
		{
			spindles.push_back(Json{{"state", "unconfigured"}});
		}

		// state
		Json messageBox = nullptr;
		if (m_messageBox)
		{
			const MessageBox& box = *m_messageBox;
			messageBox = Json{{"axisControls", box.axisControls},
							  {"cancelButton", box.cancelButton},
							  {"choices", box.choices.empty() ? Json(nullptr) : Json(box.choices)},
							  {"default", box.defaultValue ? *box.defaultValue : Json(nullptr)},
							  {"max", box.max ? Json(*box.max) : Json(nullptr)},
							  {"message", box.message},
							  {"min", box.min ? Json(*box.min) : Json(nullptr)},
							  {"mode", box.mode},
							  {"seq", box.seq},
							  {"timeout", box.timeout},
							  {"title", box.title}};
		}
		const std::time_t now = CLOCK_START + static_cast<std::time_t>(m_upTime);
		std::tm utc{};
		gmtime_r(&now, &utc);
		const Json state{{"atxPower", nullptr},
						 {"beep", nullptr},
						 {"currentTool", m_currentTool},
						 {"deferredPowerDown", nullptr},
						 {"displayMessage", m_displayMessage},
						 {"gpOut", Json::array()},
						 {"laserPwm", nullptr},
						 {"logFile", ""},
						 {"logLevel", "off"},
						 {"machineMode", modeObjectModelName(m_config.mode)},
						 {"macroRestarted", false},
						 {"messageBox", messageBox},
						 {"msUpTime", static_cast<int>(std::fmod(m_upTime, 1.0) * 1000.0)},
						 {"nextTool", m_currentTool},
						 {"powerFailScript", ""},
						 {"previousTool", m_previousTool},
						 {"restorePoints", Json::array()},
						 {"startupError", nullptr},
						 {"status", status()},
						 {"thisInput", nullptr},
						 {"time",
						  fmt::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}",
									  utc.tm_year + 1900,
									  utc.tm_mon + 1,
									  utc.tm_mday,
									  utc.tm_hour,
									  utc.tm_min,
									  utc.tm_sec)},
						 {"upTime", static_cast<int64_t>(m_upTime)}};

		// tools
		Json tools = Json::array();
		for (size_t t = 0; t < m_tools.size(); t++)
		{
			const Tool& tool = m_config.tools[t];
			Json active = Json::array();
			Json standby = Json::array();
			for (const int heater : tool.heaters)
			{
				const bool exists = heater >= 0 && static_cast<size_t>(heater) < m_heaters.size();
				active.push_back(exists ? rounded(m_heaters[static_cast<size_t>(heater)].active, 1) : 0.0);
				standby.push_back(exists ? rounded(m_heaters[static_cast<size_t>(heater)].standby, 1) : 0.0);
			}
			Json offsets = Json::array();
			for (size_t a = 0; a < m_axes.size(); a++)
			{
				offsets.push_back(0);
			}
			// Its first extruder takes all it extrudes, as M563 sets a tool up
			Json mix = Json::array();
			for (size_t e = 0; e < tool.extruders.size(); e++)
			{
				mix.push_back(e == 0 ? 1.0 : 0.0);
			}
			tools.push_back(Json{
				{"active", active},
				{"axes", Json::array({Json::array({0}), Json::array({1}), Json::array({2})})},
				{"extruders", tool.extruders},
				{"fans", tool.fans},
				{"filamentExtruder", tool.extruders.empty() ? -1 : tool.extruders.front()},
				{"heaters", tool.heaters},
				{"isRetracted", false},
				{"mix", mix},
				{"name", tool.name},
				{"number", t},
				{"offsets", offsets},
				{"offsetsProbed", 0},
				{"retraction",
				 Json{{"extraRestart", 0}, {"length", 1.0}, {"speed", 35.0}, {"unretractSpeed", 35.0}, {"zHop", 0}}},
				{"spindle", tool.spindle},
				{"spindleRpm", m_tools[t].spindleRpm},
				{"standby", standby},
				{"state", m_tools[t].state}});
		}

		return Json{{"boards", boards},
					{"directories",
					 Json{{"filaments", "0:/filaments/"},
						  {"firmware", "0:/firmware/"},
						  {"gCodes", "0:/gcodes/"},
						  {"macros", "0:/macros/"},
						  {"menu", "0:/menu/"},
						  {"system", "0:/sys/"},
						  {"web", "0:/www/"}}},
					{"fans", fans},
					{"heat", heat},
					{"inputs", inputs},
					{"job", jobModel()},
					{"move", move},
					{"network",
					 Json{{"corsSite", ""},
						  {"hostname", "simulated-duet"},
						  {"interfaces",
						   Json::array({Json{{"actualIP", "127.0.0.1"},
											 {"gateway", "0.0.0.0"},
											 {"mac", "00:00:00:00:00:00"},
											 {"state", "active"},
											 {"subnet", "255.0.0.0"},
											 {"type", "ethernet"}}})},
						  {"name", m_config.name}}},
					{"sensors", sensors},
					{"seqs", seqsModel()},
					{"spindles", spindles},
					{"state", state},
					{"tools", tools},
					{"volumes",
					 Json::array({Json{{"capacity", 16000000000LL},
									   {"freeSpace", 15000000000LL},
									   {"mounted", true},
									   {"openFiles", false},
									   {"partitionSize", 16000000000LL},
									   {"path", "0:/"},
									   {"speed", 25000000}},
								  Json{{"mounted", false}, {"path", "1:/"}}})}};
	}

	Json Machine::seqsModel() const
	{
		Json seqs = Json::object();
		for (const auto& [key, seq] : m_seqs)
		{
			seqs[key] = seq;
		}
		seqs["reply"] = m_replySeq;
		return seqs;
	}

	Json Machine::modelReply(std::string_view key, std::string_view flags)
	{
		Json model = objectModel();
		updateSeqs(model);
		model["seqs"] = seqsModel(); // with the seqs just counted

		Json reply{{"key", key}, {"flags", flags}};
		if (key.empty())
		{
			if (flags.find('f') != std::string_view::npos)
			{
				Json live = keep(model, liveFields());
				live["seqs"] = model["seqs"];
				reply["result"] = live;
			}
			else
			{
				reply["result"] = model;
			}
			return reply;
		}

		// The value at `key`, e.g. "move.axes"
		const Json* value = &model;
		std::string_view path = key;
		while (!path.empty() && value != nullptr)
		{
			const size_t dot = path.find('.');
			const std::string part(path.substr(0, dot));
			value = value->is_object() && value->contains(part) ? &(*value)[part] : nullptr;
			path = dot == std::string_view::npos ? std::string_view() : path.substr(dot + 1);
		}
		Json result = value != nullptr ? *value : Json(nullptr);

		// `move` holds only the first few axes, as a Duet's does; the rest are asked for as move.axes
		if (key == "move" && result["axes"].size() > MAX_REPORTED_AXES)
		{
			Json axes = Json::array();
			for (size_t i = 0; i < MAX_REPORTED_AXES; i++)
			{
				axes.push_back(result["axes"][i]);
			}
			result["axes"] = axes;
		}
		// An array from the index after `a`
		if (const size_t a = flags.find('a'); a != std::string_view::npos && result.is_array())
		{
			size_t first = 0;
			for (size_t i = a + 1; i < flags.size() && std::isdigit(static_cast<unsigned char>(flags[i])); i++)
			{
				first = first * 10 + static_cast<size_t>(flags[i] - '0');
			}
			Json rest = Json::array();
			for (size_t i = first; i < result.size(); i++)
			{
				rest.push_back(result[i]);
			}
			result = rest;
		}
		reply["result"] = flags.find('n') == std::string_view::npos ? withoutNulls(result) : result;
		if (result.is_array())
		{
			reply["next"] = 0;
		}
		return reply;
	}

} // namespace Sim

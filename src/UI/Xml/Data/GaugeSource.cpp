/*
 * GaugeSource.cpp
 */

#include "GaugeSource.h"
#include "Debug.h"
#include "JobSource.h"
#include "MoveSource.h"
#include "ObjectModel/Tree/Tree.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <set>

namespace UI::Xml
{
	namespace
	{
		/** The top of a percentage factor's scale: twice the normal 100%, so 100% is half way */
		constexpr int32_t FACTOR_SCALE_MAX = 200;

		/** A 0-1 fraction as a whole percentage */
		int32_t percent(float fraction)
		{
			return static_cast<int32_t>(std::lround(100.0f * fraction));
		}

		/**
		 * A heater's row: its temperature on the heater's scale, heading for `target` (0 when it isn't heating),
		 * and a detail saying the target, or its state when it has none
		 */
		GaugeRow::Values heater(std::string id,
								std::string name,
								std::string_view state,
								float current,
								int32_t target,
								float min,
								float max)
		{
			const int32_t scaleMin = static_cast<int32_t>(std::floor(std::max(0.0f, min)));
			return {.id = std::move(id),
					.name = std::move(name),
					.kind = "heater",
					.value = static_cast<int32_t>(std::lround(current)),
					.min = scaleMin,
					.max = static_cast<int32_t>(std::ceil(max)),
					.text = fmt::format("{:.1f}", current),
					.unit = "°C",
					.target = target > 0 ? target : scaleMin,
					.detail = target > 0 ? _("widget.gauge_target", target)
										 : _(fmt::format("temperature.status.{:s}", state)),
					.state = std::string(state),
					.editable = true};
		}

		/** A bed's or chamber's row; `kind` is "bed" or "chamber", `title` its translated name */
		GaugeRow::Values bedOrChamber(const PrinterModel::HeatedPlace& place,
									  const char* kind,
									  const std::string& title,
									  size_t count)
		{
			const OM::Node& h = PrinterModel::heater(place.heater);
			return heater(fmt::format("{:s}.{:d}", kind, place.number),
						  count > 1 ? fmt::format("{:s} {:d}", title, place.number) : title,
						  PrinterModel::heaterState(place.heater),
						  static_cast<float>(h["current"].number()),
						  PrinterModel::heaterTarget(place.heater),
						  static_cast<float>(h["min"].number()),
						  static_cast<float>(h["max"].number()));
		}

		/** A percentage on a scale of 0 to `max` */
		GaugeRow::Values percentage(std::string id, std::string name, int32_t value, int32_t max, bool editable)
		{
			return {.id = std::move(id),
					.name = std::move(name),
					.kind = "percent",
					.value = value,
					.min = 0,
					.max = max,
					.text = fmt::format("{:d}", value),
					.unit = "%",
					.target = 0,
					.detail = {},
					.state = {},
					.editable = editable};
		}
	} // namespace

	void GaugeRow::fields(FieldVisitor& f)
	{
		f.field("id",
				id,
				"What it shows: bed.N, chamber.N, tool.T.H (tool T's Hth heater, from 0), fan.N, sensor.N, "
				"job.progress, speed_factor or extrusion_factor (as `row.id`)");
		f.field("name", name, "Display name, e.g. \"Bed\" or \"Tool 0\"");
		f.field("kind", kind, "heater, fan, sensor or percent");
		f.field("value", value, "The value, whole units, e.g. °C or percent");
		f.field("min", min, "Bottom of its scale, e.g. 0");
		f.field("max", max, "Top of its scale: a heater's maximum, 100 for a fan or the job, 200 for a factor");
		f.field("text", text, "The value as shown, without its unit, e.g. \"61.2\"");
		f.field("unit", unit, "The value's unit, e.g. \"°C\" or \"%\"");
		f.field("target", target, "What it is heading for, e.g. a heater's target; `min` if nothing");
		f.field("detail",
				detail,
				"A line about the value, e.g. \"Target 215°C\", \"Off\" or \"1200 RPM\"; empty if there is none");
		f.field("state", state, "A heater's status key: active, standby, off, fault, tuning, offline; else empty");
		f.field("editable", editable, "1 if actions.gauge.edit changes it: a heater's target, a fan's speed, a factor");
	}

	void GaugeRow::show(const Values& values)
	{
		id.set(values.id);
		name.set(values.name);
		kind.set(values.kind);
		// The range first: a bar or arc clamps a value outside the range it has
		min.set(values.min);
		max.set(values.max);
		value.set(values.value);
		text.set(values.text);
		unit.set(values.unit);
		target.set(values.target);
		detail.set(values.detail);
		state.set(values.state);
		editable.set(values.editable ? 1 : 0);
	}

	GaugeSource::GaugeSource()
		: DataSource("gauge")
	{
	}

	void GaugeSource::fields(FieldVisitor& f)
	{
		f.list("gauges",
			   m_gauges,
			   "Every value a gauge can show, one row each, all with the same fields: the heaters, fans, analog "
			   "sensors that no heater uses, the job's progress and the speed and extrusion factors");
	}

	void GaugeSource::onInit()
	{
		watchObjectModel({"fans", "tools", "heat", "sensors.analog", "state.currentTool", "job", "move"});
	}

	void GaugeSource::update()
	{
		ZoneScoped;
		const auto rows = values();
		m_gauges.sync(rows.size(), [&rows](GaugeRow& row, size_t i) { row.show(rows[i]); });
	}

	void GaugeSource::clear()
	{
		m_gauges.clear();
	}

	std::vector<GaugeRow::Values> GaugeSource::values()
	{
		MODEL_LOCK();
		std::vector<GaugeRow::Values> rows;

		// Every tool's heaters, named after the tool, and after the heater too if the tool has several
		for (const size_t toolNumber : PrinterModel::tools())
		{
			const OM::Node& tool = PrinterModel::tool(toolNumber);
			const size_t count = tool["heaters"].size();
			for (size_t slot = 0; slot < count; ++slot)
			{
				const auto heaterNumber = PrinterModel::toolHeater(toolNumber, slot);
				if (!heaterNumber)
				{
					continue;
				}
				const OM::Node& h = PrinterModel::heater(*heaterNumber);
				std::string name = PrinterModel::toolName(toolNumber);
				if (count > 1)
				{
					name += " " + PrinterModel::heaterName(*heaterNumber);
				}
				const std::string state = PrinterModel::heaterState(*heaterNumber);
				const int32_t target = state == "active"	? static_cast<int32_t>(tool["active"][slot].integer(0))
									   : state == "standby" ? static_cast<int32_t>(tool["standby"][slot].integer(0))
															: 0;
				rows.push_back(heater(fmt::format("tool.{:d}.{:d}", toolNumber, slot),
									  std::move(name),
									  state,
									  static_cast<float>(h["current"].number()),
									  target,
									  static_cast<float>(h["min"].number()),
									  static_cast<float>(h["max"].number())));
			}
		}

		const auto beds = PrinterModel::beds();
		const std::string bed = _("temperature.bed");
		for (const auto& place : beds)
		{
			rows.push_back(bedOrChamber(place, "bed", bed, beds.size()));
		}
		const auto chambers = PrinterModel::chambers();
		const std::string chamber = _("temperature.chamber");
		for (const auto& place : chambers)
		{
			rows.push_back(bedOrChamber(place, "chamber", chamber, chambers.size()));
		}

		const OM::Node& fans = OM::Tree::get().root()["fans"];
		for (size_t number = 0; number < fans.size(); ++number)
		{
			const OM::Node& fan = fans[number];
			if (fan.isNull())
			{
				continue;
			}
			const std::string_view name = fan["name"].text();
			// A thermostatic fan follows its sensors' temperature, so it can't be set by hand
			const bool thermostatic = fan["thermostatic"]["lowTemperature"].exists();
			GaugeRow::Values row = percentage(fmt::format("fan.{:d}", number),
											  name.empty() ? _("fine_tune.fan", number) : std::string(name),
											  percent(static_cast<float>(fan["actualValue"].number())),
											  100,
											  !thermostatic);
			row.kind = "fan";
			row.target = percent(static_cast<float>(fan["requestedValue"].number()));
			const int64_t rpm = fan["rpm"].integer(-1);
			row.detail = rpm >= 0 ? _("widget.gauge_rpm", rpm) : std::string();
			rows.push_back(std::move(row));
		}

		// Sensors a heater uses are shown as that heater
		std::set<size_t> heaterSensors;
		const OM::Node& heaters = PrinterModel::root()["heat"]["heaters"];
		for (size_t number = 0; number < heaters.size(); ++number)
		{
			if (const int64_t sensor = heaters[number]["sensor"].integer(-1); sensor >= 0)
			{
				heaterSensors.insert(static_cast<size_t>(sensor));
			}
		}
		const OM::Node& sensors = PrinterModel::root()["sensors"]["analog"];
		for (size_t number = 0; number < sensors.size(); ++number)
		{
			const OM::Node& sensor = sensors[number];
			if (sensor.isNull() || heaterSensors.contains(number))
			{
				continue;
			}
			// No scale is known: 0 to 100, or to the next hundred above the reading
			const double lastReading = sensor["lastReading"].number();
			const int32_t reading = static_cast<int32_t>(std::lround(lastReading));
			const std::string_view sensorName = sensor["name"].text();
			rows.push_back({.id = fmt::format("sensor.{:d}", number),
							.name = sensorName.empty() ? _("widget.gauge_sensor", number) : std::string(sensorName),
							.kind = "sensor",
							.value = reading,
							.min = 0,
							.max = std::max(100, (reading / 100 + 1) * 100),
							.text = fmt::format("{:.1f}", static_cast<float>(lastReading)),
							.unit = "°C",
							.target = 0,
							.detail = {},
							.state = {},
							.editable = false});
		}

		rows.push_back(percentage("job.progress", _("widget.job_progress"), JobSource::progress(), 100, false));
		rows.push_back(percentage(
			"speed_factor", _("widget.status_speed"), percent(MoveSource::speedFactor()), FACTOR_SCALE_MAX, true));
		rows.push_back(percentage("extrusion_factor",
								  _("widget.status_extrusion"),
								  static_cast<int32_t>(MoveSource::extrusionFactor()),
								  FACTOR_SCALE_MAX,
								  true));
		return rows;
	}
} // namespace UI::Xml

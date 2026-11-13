/*
 * GaugeSource.cpp
 */

#include "GaugeSource.h"
#include "Debug.h"
#include "JobSource.h"
#include "MoveSource.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/BedOrChamber.h"
#include "ObjectModel/Fan.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/Sensor.h"
#include "ObjectModel/Tool.h"
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
		GaugeRow::Values heater(
			std::string id, std::string name, const char* state, float current, int32_t target, float min, float max)
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
					.state = state,
					.editable = true};
		}

		/** A bed's or chamber's row; `kind` is "bed" or "chamber", `title` its translated name */
		GaugeRow::Values bedOrChamber(OM::BedOrChamber& item, const char* kind, const std::string& title, size_t count)
		{
			return heater(fmt::format("{:s}.{:d}", kind, item.index),
						  count > 1 ? fmt::format("{:s} {:d}", title, item.index) : title,
						  item.GetHeaterStatusStr(),
						  item.GetCurrentTemp(),
						  item.GetCurrentTarget(),
						  item.GetMin(),
						  item.GetMax());
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
				"job.progress, speed_factor or extrusion_factor");
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
		f.field("editable", editable, "1 if gauges.edit changes it: a heater's target, a fan's speed, a factor");
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
		: DataSource("gauges")
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
		registerEventListener<EventType::HeaterData>(this, &GaugeSource::update);
		registerEventListener<EventType::BedHeaterData>(this, &GaugeSource::update);
		registerEventListener<EventType::ChamberHeaterData>(this, &GaugeSource::update);
		registerEventListener<EventType::ToolData>(this, &GaugeSource::update);
		registerEventListener<EventType::ToolHeaterData>([this](size_t) { update(); });
		registerEventListener<EventType::FanData>(this, &GaugeSource::update);
		registerEventListener<EventType::AnalogSensorData>(this, &GaugeSource::update);
		registerEventListener<EventType::JobDuration>(this, &GaugeSource::update);
		registerEventListener<EventType::JobFileProgress>(this, &GaugeSource::update);
		registerEventListener<EventType::SpeedFactor>(this, &GaugeSource::update);
		registerEventListener<EventType::ExtruderData>(this, &GaugeSource::update);
		registerEventListener<EventType::CurrentTool>(this, &GaugeSource::update);
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
		for (size_t slot = 0; slot < OM::GetToolCount(); ++slot)
		{
			auto tool = OM::GetToolBySlot(slot);
			if (tool == nullptr)
			{
				continue;
			}
			const size_t count = tool->GetHeaterCount();
			tool->IterateHeaters(
				[&](OM::ToolHeaterPtr toolHeater, size_t index)
				{
					if (toolHeater == nullptr || toolHeater->heater == nullptr)
					{
						return;
					}
					const auto& h = *toolHeater->heater;
					std::string name = tool->GetName();
					if (count > 1)
					{
						const std::string_view sensor = h.GetName();
						name += " " + (sensor.empty() ? _("temperature.heater", h.index) : std::string(sensor));
					}
					const int32_t target = h.status == OM::Heat::HeaterStatus::active	 ? toolHeater->activeTemp
										   : h.status == OM::Heat::HeaterStatus::standby ? toolHeater->standbyTemp
																						 : 0;
					rows.push_back(heater(fmt::format("tool.{:d}.{:d}", tool->index, index),
										  std::move(name),
										  h.GetHeaterStatusStr(),
										  h.current,
										  target,
										  h.min,
										  h.max));
				});
		}

		const size_t beds = OM::GetBedCount();
		const std::string bed = _("temperature.bed");
		for (size_t slot = 0; slot < beds; ++slot)
		{
			if (auto item = OM::GetBedBySlot(slot))
			{
				rows.push_back(bedOrChamber(*item, "bed", bed, beds));
			}
		}
		const size_t chambers = OM::GetChamberCount();
		const std::string chamber = _("temperature.chamber");
		for (size_t slot = 0; slot < chambers; ++slot)
		{
			if (auto item = OM::GetChamberBySlot(slot))
			{
				rows.push_back(bedOrChamber(*item, "chamber", chamber, chambers));
			}
		}

		for (size_t slot = 0; slot < OM::GetFanCount(); ++slot)
		{
			auto fan = OM::GetFanBySlot(slot);
			if (fan == nullptr)
			{
				continue;
			}
			GaugeRow::Values row = percentage(fmt::format("fan.{:d}", fan->index),
											  _("fine_tune.fan", fan->index),
											  percent(fan->actualValue),
											  100,
											  !fan->thermostatic);
			row.kind = "fan";
			row.target = percent(fan->requestedValue);
			row.detail = fan->rpm >= 0 ? _("widget.gauge_rpm", fan->rpm) : std::string();
			rows.push_back(std::move(row));
		}

		// Sensors a heater uses are shown as that heater
		std::set<size_t> heaterSensors;
		OM::Heat::IterateHeatersWhile(
			[&](OM::Heat::HeaterPtr h, size_t)
			{
				if (h && h->sensor)
				{
					heaterSensors.insert(h->sensor->index);
				}
				return true;
			});
		for (size_t slot = 0; slot < OM::GetAnalogSensorCount(); ++slot)
		{
			auto sensor = OM::GetAnalogSensorBySlot(slot);
			if (sensor == nullptr || heaterSensors.contains(sensor->index))
			{
				continue;
			}
			// No scale is known: 0 to 100, or to the next hundred above the reading
			const int32_t reading = static_cast<int32_t>(std::lround(sensor->lastReading));
			rows.push_back({.id = fmt::format("sensor.{:d}", sensor->index),
							.name = sensor->name.empty() ? _("widget.gauge_sensor", sensor->index) : sensor->name,
							.kind = "sensor",
							.value = reading,
							.min = 0,
							.max = std::max(100, (reading / 100 + 1) * 100),
							.text = fmt::format("{:.1f}", sensor->lastReading),
							.unit = "°C",
							.target = 0,
							.detail = {},
							.state = {},
							.editable = false});
		}

		rows.push_back(percentage("job.progress", _("widget.job_progress"), JobSource::progress(), 100, false));
		rows.push_back(percentage(
			"speed_factor", _("widget.status_speed"), percent(OM::Move::GetSpeedFactor()), FACTOR_SCALE_MAX, true));
		rows.push_back(percentage("extrusion_factor",
								  _("widget.status_extrusion"),
								  static_cast<int32_t>(MoveSource::extrusionFactor()),
								  FACTOR_SCALE_MAX,
								  true));
		return rows;
	}
} // namespace UI::Xml

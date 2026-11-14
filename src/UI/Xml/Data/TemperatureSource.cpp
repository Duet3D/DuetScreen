/*
 * TemperatureSource.cpp
 */

#include "TemperatureSource.h"
#include "Configuration.h"
#include "Debug.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <vector>

namespace UI::Xml
{
	namespace
	{
		constexpr size_t HISTORY_SECONDS = 300; // How far back the history goes
	} // namespace

	TemperatureSource::TemperatureSource()
		: DataSource("temperatures")
		, m_history(MODEL_TICK_HZ, HISTORY_SECONDS * MODEL_TICK_HZ)
	{
	}

	void TemperatureSource::fields(FieldVisitor& f)
	{
		f.list("temperatures",
			   m_history,
			   "One series per heater, in slot order, sampled every model tick, whole °C. Heaters sharing a sensor are "
			   "shown once.");
		f.field("temperatures.limit",
				m_limit,
				"Highest maximum temperature of any heater, °C (0 with no heaters), e.g. for a graph's axis");
	}

	std::vector<TemperatureSource::Shown> TemperatureSource::heatersToShow()
	{
		std::vector<Shown> shown;
		const OM::Node& heaters = PrinterModel::root()["heat"]["heaters"];
		const OM::Node& sensors = PrinterModel::root()["sensors"]["analog"];
		for (size_t number = 0; number < heaters.size(); number++)
		{
			const int64_t sensor = heaters[number]["sensor"].integer(-1);
			if (sensor < 0 || !sensors[static_cast<size_t>(sensor)].exists() ||
				std::any_of(
					shown.begin(),
					shown.end(),
					[sensor](const Shown& s) { return s.sensor == static_cast<size_t>(sensor); }))
			{
				continue;
			}
			shown.push_back({.heater = number, .sensor = static_cast<size_t>(sensor)});
		}
		return shown;
	}

	void TemperatureSource::onInit()
	{
		registerEventListener<EventType::Tick>(this, &TemperatureSource::sample);
		watchObjectModel({"heat.heaters", "sensors.analog"});
	}

	void TemperatureSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		syncSeries(heatersToShow());
	}

	void TemperatureSource::syncSeries(const std::vector<Shown>& heaters)
	{
		int32_t limit = 0;
		m_history.sync(heaters.size(),
					   [&](SeriesRow& row, size_t i)
					   {
						   row.name.set(PrinterModel::heaterName(heaters[i].heater));
						   limit = std::max(
							   limit, static_cast<int32_t>(PrinterModel::heater(heaters[i].heater)["max"].number()));
					   });
		m_limit.set(limit);
	}

	void TemperatureSource::clear()
	{
		m_history.clear();
		m_limit.set(0);
	}

	void TemperatureSource::sample()
	{
		ZoneScoped;
		MODEL_LOCK();
		const auto heaters = heatersToShow();
		syncSeries(heaters);
		const OM::Node& sensors = PrinterModel::root()["sensors"]["analog"];
		m_history.record([&](SeriesRow&, size_t i)
						 { return static_cast<int32_t>(sensors[heaters[i].sensor]["lastReading"].number()); });
	}
} // namespace UI::Xml

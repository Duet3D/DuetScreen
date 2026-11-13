/*
 * TemperatureSource.cpp
 */

#include "TemperatureSource.h"
#include "Configuration.h"
#include "Debug.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/Sensor.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <vector>

namespace UI::Xml
{
	namespace
	{
		constexpr size_t HISTORY_SECONDS = 300; // How far back the history goes

		/** The heaters to show, in slot order: those with a sensor, each sensor once */
		std::vector<OM::Heat::HeaterPtr> heatersToShow()
		{
			std::vector<OM::Heat::HeaterPtr> heaters;
			std::vector<size_t> sensors;
			for (size_t slot = 0; slot < OM::Heat::GetHeaterCount(); slot++)
			{
				OM::Heat::HeaterPtr heater = OM::Heat::GetHeaterBySlot(slot);
				if (heater == nullptr || heater->sensor == nullptr ||
					std::find(sensors.begin(), sensors.end(), heater->sensor->index) != sensors.end())
				{
					continue;
				}
				sensors.push_back(heater->sensor->index);
				heaters.push_back(heater);
			}
			return heaters;
		}
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

	void TemperatureSource::onInit()
	{
		registerEventListener<EventType::Tick>(this, &TemperatureSource::sample);
	}

	void TemperatureSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		syncSeries(heatersToShow());
	}

	void TemperatureSource::syncSeries(const std::vector<OM::Heat::HeaterPtr>& heaters)
	{
		int32_t limit = 0;
		m_history.sync(heaters.size(),
					   [&](SeriesRow& row, size_t i)
					   {
						   const OM::Heat::Heater& heater = *heaters[i];
						   const std::string name(heater.GetName());
						   row.name.set(name.empty() ? _("temperature.heater", heater.index) : name);
						   limit = std::max(limit, static_cast<int32_t>(heater.max));
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
		m_history.record([&](SeriesRow&, size_t i) { return static_cast<int32_t>(heaters[i]->sensor->lastReading); });
	}
} // namespace UI::Xml

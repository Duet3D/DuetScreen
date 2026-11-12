/*
 * TemperatureSource.h
 *
 * Records recent temperatures, for graphs.
 *
 *   temperatures        history (see Binding/History.h), one series per heater in slot order, sampled every model
 *                       tick. Heaters sharing a sensor are shown once.
 *     index             position in the list
 *     name              heater name, or "Heater N" if it has none
 *   temperatures.limit  highest maximum temperature of any heater, °C (0 with no heaters), so a graph's axis can
 *                       reach it: `<ds_graph bind_y_max="temperatures.limit"/>`
 *
 * Values are whole degrees Celsius.
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/History.h"
#include "ObjectModel/Heat.h"
#include "UI/Xml/Binding/Subjects.h"
#include <vector>

namespace UI::Xml
{
	class TemperatureSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		TemperatureSource();

		/** Declare `temperatures` and `temperatures.limit` */
		void fields(FieldVisitor& f) override;

		/** Re-read which heaters there are, their names and limits */
		void update() override;

		/** Forget every series and its history */
		void clear() override;

		/** Record every heater's current temperature. Called every model tick. */
		void sample();

	  protected:
		/** Sample every model tick */
		void onInit() override;

	  private:
		/** Make the series match `heaters`, and update the limit */
		void syncSeries(const std::vector<OM::Heat::HeaterPtr>& heaters);

		History m_history; // One series per heater
		IntSubject m_limit; // Highest heater maximum, °C
	};
} // namespace UI::Xml

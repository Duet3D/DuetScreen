/*
 * TemperatureSource.h
 *
 * Records recent temperatures, for graphs: a history (see Binding/History.h) and the highest temperature a graph's
 * axis needs to reach, e.g. `<ds_graph bind_y_max="temperatures.limit"/>`.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/History.h"
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
		/** Sample every model tick, and follow the heaters and sensors' names */
		void onInit() override;

	  private:
		/** A heater shown */
		struct Shown
		{
			size_t heater; // Heater number
			size_t sensor; // Its sensor's number
		};

		/** The heaters to show, in order: those with a sensor, each sensor once (the model lock held) */
		static std::vector<Shown> heatersToShow();

		/** Make the series match `heaters`, and update the limit (the model lock held) */
		void syncSeries(const std::vector<Shown>& heaters);

		History m_history; // One series per heater
		IntSubject m_limit; // Highest heater maximum, °C
	};
} // namespace UI::Xml

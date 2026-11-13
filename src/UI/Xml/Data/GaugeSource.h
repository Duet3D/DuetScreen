/*
 * GaugeSource.h
 *
 * Publishes `gauges`: every single value a gauge widget can show, one row each, all with the same fields, so one
 * component shows any of them and the user picks which in the widget's settings (a `row` setting keyed on `id`):
 *
 *   - each heater: every tool's heaters, the beds and the chambers, in °C, with the target it heats to
 *   - each fan's speed, in percent, and each analog sensor that no heater uses
 *   - the job's progress, and the speed and extrusion factors, in percent
 *
 * A row's `id` names what it shows by what the user configured (`bed.0`, `tool.1.0`, `fan.2`), so a widget keeps
 * showing the same thing as other rows come and go. `gauges.edit` (see UI/Presenters/Gauge/GaugeActions.h) changes
 * a row's value where that means something: a heater's target, a fan's speed, a factor.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include <string>
#include <vector>

namespace UI::Xml
{
	/** One value a gauge can show */
	class GaugeRow : public Row
	{
	  public:
		/** What a row shows */
		struct Values
		{
			std::string id;		   // What it shows, e.g. "bed.0" (see the file comment)
			std::string name;	   // Display name, e.g. "Bed"
			std::string kind;	   // heater, fan, sensor or percent
			int32_t value = 0;	   // The value, whole units
			int32_t min = 0;	   // Bottom of its scale
			int32_t max = 100;	   // Top of its scale
			std::string text;	   // The value as shown, without its unit, e.g. "61.2"
			std::string unit;	   // e.g. "°C" or "%"
			int32_t target = 0;	   // What it is heading for; `min` if nothing
			std::string detail;	   // A line about it, e.g. "Target 215°C" or "1200 RPM"; empty for none
			std::string state;	   // A heater's status key (active, standby, off, fault, …); empty otherwise
			bool editable = false; // Whether gauges.edit changes it
		};

		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		/** Show `values` */
		void show(const Values& values);

		StringSubject<24> id;		// What it shows, e.g. "bed.0"
		StringSubject<48> name;		// Display name
		StringSubject<16> kind;		// heater, fan, sensor or percent
		IntSubject value;			// The value
		IntSubject min;				// Bottom of its scale
		IntSubject max{100};		// Top of its scale
		StringSubject<16> text;		// The value as shown, without its unit
		StringSubject<8> unit;		// Its unit
		IntSubject target;			// What it is heading for; `min` if nothing
		StringSubject<48> detail;	// A line about it; empty for none
		StringSubject<16> state;	// A heater's status key; empty otherwise
		IntSubject editable;		// 1 if gauges.edit changes it
	};

	/** Publishes the `gauges` list */
	class GaugeSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		GaugeSource();

		/** Declare the `gauges` list */
		void fields(FieldVisitor& f) override;

		/** Re-read every value */
		void update() override;

		/** Empty the list */
		void clear() override;

	  protected:
		/** Listen for heater, fan, sensor, job and override changes */
		void onInit() override;

	  private:
		/** Every value there is now, in the order shown: heaters, fans, sensors, then the job and the factors */
		static std::vector<GaugeRow::Values> values();

		List<GaugeRow> m_gauges; // One row per value
	};
} // namespace UI::Xml

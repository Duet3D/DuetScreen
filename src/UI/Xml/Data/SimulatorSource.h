/*
 * SimulatorSource.h
 *
 * Publishes what the simulated Duet (src/Simulator) is made of, for its page in the settings (Settings > Connection >
 * Simulated Duet): a list each of its axes, heaters, extruders, tools, fans, other sensors, spindles and jobs, every row with
 * its own values to edit. The global actions `actions.simulator.*` (src/UI/Presenters/Settings/SimulatorActions.h)
 * add, remove and change them; what applies to the whole machine (its preset, name, mode, status, and how it
 * behaves as a connection) are settings, `source.settings.simulator_*` (SimulatorSettings.cpp).
 *
 * It is in every build, so the UI API is the same in all; in a build without the simulated Duet (DUET_SIMULATOR) its
 * lists are empty and `source.system.simulator_shown` is 0.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml
{
	/** The lists of the simulated Duet's parts */
	enum class SimulatorPart
	{
		axes,
		heaters,
		extruders,
		tools,
		fans,
		sensors,
		spindles,
		files,
	};

	/** A part of the simulated Duet: which list it is in, and where */
	struct SimulatorItem
	{
		SimulatorPart part = SimulatorPart::axes; // Its list
		size_t index = 0;						  // Its place in the list, which is its number
	};

	/** A row of one of the parts' lists: which part, and its title */
	class SimulatorRow : public Row
	{
	  public:
		/** Declare the fields every part's row has, then the part's own */
		void fields(FieldVisitor& f) override;

		/** The part it shows */
		SimulatorItem item() const { return {.part = part, .index = static_cast<size_t>(index.get())}; }

		/** Declare the part's own fields */
		virtual void partFields(FieldVisitor& f) = 0;

		SimulatorPart part = SimulatorPart::axes; // Which list it is in
		StringSubject<48> title;				  // E.g. "Heater 1"
	};

	/** An axis */
	class SimulatorAxisRow : public SimulatorRow
	{
	  public:
		/** Declare the axis's fields */
		void partFields(FieldVisitor& f) override;

		IntSubject letter;		 // Index of its letter in source.simulator.axis_letters
		StringSubject<24> min;	 // Its lowest position, e.g. "0 mm"
		StringSubject<24> max;	 // Its highest
	};

	/** A heater */
	class SimulatorHeaterRow : public SimulatorRow
	{
	  public:
		/** Declare the heater's fields */
		void partFields(FieldVisitor& f) override;

		StringSubject<64> name; // Its sensor's name
		IntSubject role;		// Index of what it heats, in source.simulator.heater_roles
		StringSubject<24> max;	// Its highest temperature, e.g. "285 °C"
		StringSubject<24> heatingRate; // How fast it heats, e.g. "10 °C/s"
		StringSubject<24> coolingRate; // How fast it cools, e.g. "4 °C/s"
		IntSubject fault;			   // 1 if it is in fault
	};

	/** An extruder */
	class SimulatorExtruderRow : public SimulatorRow
	{
	  public:
		/** Declare the extruder's fields */
		void partFields(FieldVisitor& f) override;

		StringSubject<64> filament; // The filament loaded when the machine starts; "" for none
	};

	/** A heater, an extruder or a fan a tool may have: a chip, chosen if the tool has it */
	class SimulatorChoiceRow : public Row
	{
	  public:
		/** Declare the chip's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<16> text; // E.g. "H1"
		IntSubject chosen;		// 1 if the tool has it
		SimulatorPart part = SimulatorPart::heaters; // Heaters, extruders or fans
	};

	/** A tool */
	class SimulatorToolRow : public SimulatorRow
	{
	  public:
		/** Declare the tool's fields */
		void partFields(FieldVisitor& f) override;

		StringSubject<64> name;				// Its name
		IntSubject spindle;					// Index of its spindle in source.simulator.spindle_options: 0 for none
		List<SimulatorChoiceRow> heaters;	// Every heater, chosen if the tool has it
		List<SimulatorChoiceRow> extruders; // Every extruder, chosen if the tool has it
		List<SimulatorChoiceRow> fans;		// Every fan, chosen if the tool has it
	};

	/** A fan */
	class SimulatorFanRow : public SimulatorRow
	{
	  public:
		/** Declare the fan's fields */
		void partFields(FieldVisitor& f) override;

		StringSubject<64> name;	 // Its name
		IntSubject tacho;		 // 1 if it reports its rpm
		IntSubject thermostatic; // 1 if it follows the first tool's heater
	};

	/** A sensor that isn't a heater's */
	class SimulatorSensorRow : public SimulatorRow
	{
	  public:
		/** Declare the sensor's fields */
		void partFields(FieldVisitor& f) override;

		IntSubject kind;		 // Index of what it is, in source.simulator.sensor_kinds
		StringSubject<64> name;	 // Its name
		StringSubject<24> value; // Its reading, e.g. "25 °C" or "1"
	};

	/** A spindle */
	class SimulatorSpindleRow : public SimulatorRow
	{
	  public:
		/** Declare the spindle's fields */
		void partFields(FieldVisitor& f) override;

		StringSubject<24> max; // Its highest speed, e.g. "24 000 rpm"
		IntSubject reverse;	   // 1 if it can turn in reverse
	};

	/** A job on the simulated SD card */
	class SimulatorFileRow : public SimulatorRow
	{
	  public:
		/** Declare the job's fields */
		void partFields(FieldVisitor& f) override;

		StringSubject<96> name;		  // Its file name
		StringSubject<24> printTime;  // How long it takes, e.g. "60 min"
		StringSubject<16> layers;	  // How many layers it has
		IntSubject thumbnail;		  // 1 if it has thumbnails
	};

	/** A number asked for on the number pad, to change a part's value */
	struct SimulatorNumber
	{
		float value = 0.0f;	 // What it is now
		float min = 0.0f;	 // Smallest it may be
		float max = 0.0f;	 // Largest
		std::string unit{};	 // E.g. "°C"
	};

	/** Publishes `source.simulator.*` */
	class SimulatorSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		SimulatorSource();

		/** Declare `source.simulator.*` */
		void fields(FieldVisitor& f) override;

		/** Show the simulated Duet's parts as they are */
		void update() override;

		/** Its parts don't depend on being connected to it: show them still */
		void clear() override { update(); }

		/** Add a part to `part`'s list, with values to start from */
		void add(SimulatorPart part);

		/** Remove `item`; the tools that used it (a heater, fan or spindle) no longer do */
		void remove(SimulatorItem item);

		/** Give `item`'s `field` the text `text` (a name, a filament); false if it isn't such a field */
		bool setText(SimulatorItem item, std::string_view field, std::string_view text);

		/** Give `item`'s `field` the number `value` (a number, an index of a choice, 0 or 1); false if it has none */
		bool setNumber(SimulatorItem item, std::string_view field, float value);

		/** Turn `item`'s on/off `field` over; false if it isn't one */
		bool toggle(SimulatorItem item, std::string_view field);

		/** Give tool `tool` the heater, extruder or fan `choice` (`index` its number), or take it away */
		void toggle(size_t tool, SimulatorItem choice);

		/** `item`'s number `field` as the number pad asks for it, or nothing if it isn't a number */
		std::optional<SimulatorNumber> number(SimulatorItem item, std::string_view field) const;

		/** Whether `field` of a part's row is a text (a name, a filament); the others are numbers */
		static bool isText(std::string_view field);

		/** `name` ("axes", "heaters", …) as a part, or nothing */
		static std::optional<SimulatorPart> partNamed(std::string_view name);

	  private:
		StringSubject<64> m_axisLetters;	   // `axis_letters`
		StringSubject<256> m_heaterRoles;	   // `heater_roles`
		StringSubject<256> m_sensorKinds;	   // `sensor_kinds`
		StringSubject<512> m_spindleOptions;   // `spindle_options`
		IntSubject m_heaterCount;			   // `heater_count`
		IntSubject m_extruderCount;			   // `extruder_count`
		IntSubject m_fanCount;				   // `fan_count`
		List<SimulatorAxisRow> m_axes;		   // `axes`
		List<SimulatorHeaterRow> m_heaters;	   // `heaters`
		List<SimulatorExtruderRow> m_extruders; // `extruders`
		List<SimulatorToolRow> m_tools;		   // `tools`
		List<SimulatorFanRow> m_fans;		   // `fans`
		List<SimulatorSensorRow> m_sensors;	   // `sensors`
		List<SimulatorSpindleRow> m_spindles;  // `spindles`
		List<SimulatorFileRow> m_files;		   // `files`
	};
} // namespace UI::Xml

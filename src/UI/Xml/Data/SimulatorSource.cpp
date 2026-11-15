/*
 * SimulatorSource.cpp
 */

#include "SimulatorSource.h"
#include "SettingsSource.h"
#include "SpindleSource.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fmt/format.h>
#if DUET_SIMULATOR
#  include "Simulator/Simulator.h"
#endif

namespace UI::Xml
{
	namespace
	{
		/** The parts' names, as actions.simulator.add takes them, in SimulatorPart's order */
		constexpr std::string_view PART_NAMES[] = {
			"axes", "heaters", "extruders", "tools", "fans", "sensors", "spindles", "files"};

#if DUET_SIMULATOR
		/** `value` with `unit`, as few decimals as it needs, e.g. "0.5 °C" */
		std::string withUnit(float value, std::string_view unit)
		{
			return fmt::format("{:g} {:s}", std::round(value * 100.0f) / 100.0f, unit);
		}

		/** Change the simulated Duet's configuration with `change`, which it then becomes and keeps */
		template <typename F>
		void edit(F&& change)
		{
			Sim::Config config = Sim::Simulator::get().config();
			change(config);
			Sim::Simulator::get().setConfig(config);
		}

		/** The unit of a sensor of `kind`'s reading */
		std::string_view sensorUnit(Sim::SensorKind kind)
		{
			return kind == Sim::SensorKind::temperature ? "°C" : "";
		}

		/** `numbers` without `removed`, and those above it one less (a heater or fan was removed) */
		void renumber(std::vector<int>& numbers, int removed)
		{
			std::erase(numbers, removed);
			for (int& number : numbers)
			{
				number -= number > removed ? 1 : 0;
			}
		}
#endif
	} // namespace

	void SimulatorRow::fields(FieldVisitor& f)
	{
		f.field("title", title, "What it is and its number, e.g. \"Heater 1\"");
		partFields(f);
	}

	void SimulatorAxisRow::partFields(FieldVisitor& f)
	{
		f.field("letter",
				letter,
				"Index of its letter in source.simulator.axis_letters; choosing another axis's swaps them "
				"(actions.simulator.set arg=\"letter\")");
		f.field("min", min, "Its lowest position, e.g. \"0 mm\" (actions.simulator.set arg=\"min\")");
		f.field("max", max, "Its highest position, e.g. \"300 mm\" (actions.simulator.set arg=\"max\")");
	}

	void SimulatorHeaterRow::partFields(FieldVisitor& f)
	{
		f.field("name", name, "Its sensor's name (actions.simulator.set arg=\"name\")");
		f.field("role",
				role,
				"Index of what it heats in source.simulator.heater_roles: a tool, the bed, the chamber or something "
				"else (actions.simulator.set arg=\"role\")");
		f.field("max", max, "The highest temperature it may be set to, e.g. \"285 °C\" (actions.simulator.set)");
		f.field(
			"heating_rate", heatingRate, "How fast it heats at full power, e.g. \"10 °C/s\" (actions.simulator.set)");
		f.field("cooling_rate", coolingRate, "How fast it cools at most, e.g. \"4 °C/s\" (actions.simulator.set)");
		f.field("fault", fault, "1 if it is in fault: it reads, but can't heat (actions.simulator.set)");
	}

	void SimulatorExtruderRow::partFields(FieldVisitor& f)
	{
		f.field("filament",
				filament,
				"The filament loaded when the machine starts, e.g. \"PLA\"; empty for none (actions.simulator.set "
				"arg=\"filament\")");
	}

	void SimulatorChoiceRow::fields(FieldVisitor& f)
	{
		f.field("text", text, "Which heater (\"H1\"), extruder (\"E0\") or fan (\"F0\") it is");
		f.field("chosen", chosen, "1 if the tool has it (actions.simulator.toggle)");
	}

	void SimulatorToolRow::partFields(FieldVisitor& f)
	{
		f.field("name", name, "Its name (actions.simulator.set arg=\"name\")");
		f.field("spindle",
				spindle,
				"Index of its spindle in source.simulator.spindle_options; 0 for none (actions.simulator.set "
				"arg=\"spindle\")");
		f.list("heaters", heaters, "Every heater, chosen if the tool has it");
		f.list("extruders",
			   extruders,
			   "Every extruder, chosen if the tool has it; its first is the one its filament is loaded into");
		f.list("fans", fans, "Every fan, chosen if the tool has it");
	}

	void SimulatorFanRow::partFields(FieldVisitor& f)
	{
		f.field("name", name, "Its name (actions.simulator.set arg=\"name\")");
		f.field("tacho", tacho, "1 if it reports its rpm (actions.simulator.set)");
		f.field("thermostatic",
				thermostatic,
				"1 if it turns itself on while the first tool's heater is hot (actions.simulator.set)");
	}

	void SimulatorSensorRow::partFields(FieldVisitor& f)
	{
		f.field("kind",
				kind,
				"Index of what it is in source.simulator.sensor_kinds: a temperature sensor, a filament monitor, a "
				"probe or an input (actions.simulator.set arg=\"kind\")");
		f.field("name", name, "Its name (actions.simulator.set arg=\"name\")");
		f.field("value",
				value,
				"Its reading: °C, 1 if filament is present, the probe's value or the input's state "
				"(actions.simulator.set arg=\"value\")");
	}

	void SimulatorSpindleRow::partFields(FieldVisitor& f)
	{
		f.field("max", max, "Its highest speed, e.g. \"24 000 rpm\" (actions.simulator.set arg=\"max\")");
		f.field("reverse", reverse, "1 if it can turn in reverse (actions.simulator.set)");
	}

	void SimulatorFileRow::partFields(FieldVisitor& f)
	{
		f.field("name", name, "Its file name, in 0:/gcodes (actions.simulator.set arg=\"name\")");
		f.field("print_time", printTime, "How long it takes, e.g. \"60 min\" (actions.simulator.set)");
		f.field("layers", layers, "How many layers it has (actions.simulator.set)");
		f.field("thumbnail", thumbnail, "1 if it has thumbnails, as a slicer embeds them (actions.simulator.set)");
	}

	SimulatorSource::SimulatorSource()
		: DataSource("simulator")
	{
	}

	void SimulatorSource::fields(FieldVisitor& f)
	{
		f.field("axis_letters", m_axisLetters, "The letters an axis can have, one per line");
		f.field("heater_roles", m_heaterRoles, "What a heater can heat, one per line, translated");
		f.field("sensor_kinds", m_sensorKinds, "What a sensor can be, one per line, translated");
		f.field("spindle_options", m_spindleOptions, "The spindles a tool can have, one per line: none, then each");
		f.field("heater_count", m_heaterCount, "How many heaters it has");
		f.field("extruder_count", m_extruderCount, "How many extruders it has");
		f.field("fan_count", m_fanCount, "How many fans it has");
		f.list("axes", m_axes, "Its axes, in order: the first three are X, Y and Z");
		f.list("heaters", m_heaters, "Its heaters, by number, each with a sensor of its own");
		f.list("extruders", m_extruders, "Its extruders, by number");
		f.list("tools", m_tools, "Its tools, by number");
		f.list("fans", m_fans, "Its fans, by number");
		f.list("sensors", m_sensors, "Its sensors besides its heaters'");
		f.list("spindles", m_spindles, "Its spindles, by number");
		f.list("files", m_files, "The jobs on its SD card, in 0:/gcodes");
	}

	void SimulatorSource::update()
	{
		m_heaterRoles.set(Settings::lines({_("simulator.roles.tool"),
										   _("simulator.roles.bed"),
										   _("simulator.roles.chamber"),
										   _("simulator.roles.other")}));
		m_sensorKinds.set(Settings::lines({_("simulator.kinds.temperature"),
										   _("simulator.kinds.filament_monitor"),
										   _("simulator.kinds.probe"),
										   _("simulator.kinds.input")}));
#if DUET_SIMULATOR
		const Sim::Config config = Sim::Simulator::get().config();

		m_axisLetters.set(
			Settings::lines(std::vector<std::string>(Sim::axisLetters().begin(), Sim::axisLetters().end())));

		std::vector<std::string> spindles{_("simulator.no_spindle")};
		for (size_t s = 0; s < config.spindles.size(); s++)
		{
			spindles.push_back(_("simulator.titles.spindle", s));
		}
		m_spindleOptions.set(Settings::lines(spindles));
		m_heaterCount.set(static_cast<int32_t>(config.heaters.size()));
		m_extruderCount.set(static_cast<int32_t>(config.extruders.size()));
		m_fanCount.set(static_cast<int32_t>(config.fans.size()));

		m_axes.sync(config.axes.size(),
					[&config](SimulatorAxisRow& row, size_t i)
					{
						const Sim::Axis& axis = config.axes[i];
						row.part = SimulatorPart::axes;
						row.title.set(_("simulator.titles.axis", i));
						const auto letters = Sim::axisLetters();
						const auto letter = std::ranges::find(letters, axis.letter);
						row.letter.set(letter == letters.end() ? -1 : static_cast<int32_t>(letter - letters.begin()));
						row.min.set(withUnit(axis.min, "mm"));
						row.max.set(withUnit(axis.max, "mm"));
					});
		m_heaters.sync(config.heaters.size(),
					   [&config](SimulatorHeaterRow& row, size_t i)
					   {
						   const Sim::Heater& heater = config.heaters[i];
						   row.part = SimulatorPart::heaters;
						   row.title.set(_("simulator.titles.heater", i));
						   row.name.set(heater.name);
						   row.role.set(static_cast<int32_t>(heater.role));
						   row.max.set(withUnit(heater.max, "°C"));
						   row.heatingRate.set(withUnit(heater.heatingRate, "°C/s"));
						   row.coolingRate.set(withUnit(heater.coolingRate, "°C/s"));
						   row.fault.set(heater.fault ? 1 : 0);
					   });
		m_extruders.sync(config.extruders.size(),
						 [&config](SimulatorExtruderRow& row, size_t i)
						 {
							 row.part = SimulatorPart::extruders;
							 row.title.set(_("simulator.titles.extruder", i));
							 row.filament.set(config.extruders[i].filament);
						 });
		m_tools.sync(
			config.tools.size(),
			[&config](SimulatorToolRow& row, size_t i)
			{
				const Sim::Tool& tool = config.tools[i];
				row.part = SimulatorPart::tools;
				row.title.set(_("simulator.titles.tool", i));
				row.name.set(tool.name);
				row.spindle.set(tool.spindle + 1);
				row.heaters.sync(config.heaters.size(),
								 [&tool](SimulatorChoiceRow& choice, size_t h)
								 {
									 choice.part = SimulatorPart::heaters;
									 choice.text.set(fmt::format("H{:d}", h));
									 choice.chosen.set(std::ranges::contains(tool.heaters, static_cast<int>(h)));
								 });
				row.extruders.sync(config.extruders.size(),
								   [&tool](SimulatorChoiceRow& choice, size_t e)
								   {
									   choice.part = SimulatorPart::extruders;
									   choice.text.set(fmt::format("E{:d}", e));
									   choice.chosen.set(std::ranges::contains(tool.extruders, static_cast<int>(e)));
								   });
				row.fans.sync(config.fans.size(),
							  [&tool](SimulatorChoiceRow& choice, size_t fan)
							  {
								  choice.part = SimulatorPart::fans;
								  choice.text.set(fmt::format("F{:d}", fan));
								  choice.chosen.set(std::ranges::contains(tool.fans, static_cast<int>(fan)));
							  });
			});
		m_fans.sync(config.fans.size(),
					[&config](SimulatorFanRow& row, size_t i)
					{
						const Sim::Fan& fan = config.fans[i];
						row.part = SimulatorPart::fans;
						row.title.set(_("simulator.titles.fan", i));
						row.name.set(fan.name);
						row.tacho.set(fan.tacho ? 1 : 0);
						row.thermostatic.set(fan.thermostatic ? 1 : 0);
					});
		m_sensors.sync(config.sensors.size(),
					   [&config](SimulatorSensorRow& row, size_t i)
					   {
						   const Sim::Sensor& sensor = config.sensors[i];
						   row.part = SimulatorPart::sensors;
						   row.title.set(_("simulator.titles.sensor", i));
						   row.kind.set(static_cast<int32_t>(sensor.kind));
						   row.name.set(sensor.name);
						   row.value.set(withUnit(sensor.value, sensorUnit(sensor.kind)));
					   });
		m_spindles.sync(config.spindles.size(),
						[&config](SimulatorSpindleRow& row, size_t i)
						{
							const Sim::Spindle& spindle = config.spindles[i];
							row.part = SimulatorPart::spindles;
							row.title.set(_("simulator.titles.spindle", i));
							row.max.set(SpindleSource::rpmText(spindle.max) + " rpm");
							row.reverse.set(spindle.canReverse ? 1 : 0);
						});
		m_files.sync(config.files.size(),
					 [&config](SimulatorFileRow& row, size_t i)
					 {
						 const Sim::JobFile& file = config.files[i];
						 row.part = SimulatorPart::files;
						 row.title.set(_("simulator.titles.file", i));
						 row.name.set(file.name);
						 row.printTime.set(withUnit(static_cast<float>(file.printTime) / 60.0f, "min"));
						 row.layers.set(fmt::format("{:d}", file.layers));
						 row.thumbnail.set(file.thumbnail ? 1 : 0);
					 });
#else
		m_spindleOptions.set(_("simulator.no_spindle"));
#endif
	}

	std::optional<SimulatorPart> SimulatorSource::partNamed(std::string_view name)
	{
		for (size_t i = 0; i < std::size(PART_NAMES); i++)
		{
			if (PART_NAMES[i] == name)
			{
				return static_cast<SimulatorPart>(i);
			}
		}
		return std::nullopt;
	}

	bool SimulatorSource::isText(std::string_view field)
	{
		return field == "name" || field == "filament";
	}

#if DUET_SIMULATOR
	void SimulatorSource::add(SimulatorPart part)
	{
		edit(
			[part](Sim::Config& config)
			{
				switch (part)
				{
				case SimulatorPart::axes:
				{
					const std::string letter = Sim::nextAxisLetter(config.axes);
					if (letter.empty())
					{
						return;
					}
					const bool rotary = letter == "A" || letter == "B" || letter == "C";
					config.axes.push_back({.letter = letter, .min = 0.0f, .max = rotary ? 360.0f : 300.0f});
					break;
				}
				case SimulatorPart::heaters:
					config.heaters.push_back({.name = _("simulator.titles.heater", config.heaters.size())});
					break;
				case SimulatorPart::extruders:
					config.extruders.push_back({});
					break;
				case SimulatorPart::tools:
					config.tools.push_back({.name = _("simulator.titles.tool", config.tools.size())});
					break;
				case SimulatorPart::fans:
					config.fans.push_back({.name = _("simulator.titles.fan", config.fans.size())});
					break;
				case SimulatorPart::sensors:
					config.sensors.push_back({.name = _("simulator.titles.sensor", config.sensors.size())});
					break;
				case SimulatorPart::spindles:
					config.spindles.push_back({});
					break;
				case SimulatorPart::files:
					config.files.push_back({.name = fmt::format("job {:d}.gcode", config.files.size() + 1)});
					break;
				}
			});
		update();
	}

	void SimulatorSource::remove(SimulatorItem item)
	{
		const size_t index = item.index;
		edit(
			[item, index](Sim::Config& config)
			{
				const auto erase = [index](auto& items)
				{
					if (index < items.size())
					{
						items.erase(items.begin() + static_cast<std::ptrdiff_t>(index));
					}
				};
				const int number = static_cast<int>(index);
				switch (item.part)
				{
				case SimulatorPart::axes:
					erase(config.axes);
					break;
				case SimulatorPart::heaters:
					erase(config.heaters);
					for (Sim::Tool& tool : config.tools)
					{
						renumber(tool.heaters, number);
					}
					break;
				case SimulatorPart::extruders:
					erase(config.extruders);
					for (Sim::Tool& tool : config.tools)
					{
						renumber(tool.extruders, number);
					}
					break;
				case SimulatorPart::tools:
					erase(config.tools);
					break;
				case SimulatorPart::fans:
					erase(config.fans);
					for (Sim::Tool& tool : config.tools)
					{
						renumber(tool.fans, number);
					}
					break;
				case SimulatorPart::sensors:
					erase(config.sensors);
					break;
				case SimulatorPart::spindles:
					erase(config.spindles);
					for (Sim::Tool& tool : config.tools)
					{
						tool.spindle = tool.spindle == number ? -1 : tool.spindle - (tool.spindle > number ? 1 : 0);
					}
					break;
				case SimulatorPart::files:
					erase(config.files);
					break;
				}
			});
		update();
	}

	bool SimulatorSource::setText(SimulatorItem item, std::string_view field, std::string_view text)
	{
		const size_t i = item.index;
		bool changed = false;
		edit(
			[&](Sim::Config& config)
			{
				switch (item.part)
				{
				case SimulatorPart::extruders:
					if (field == "filament" && i < config.extruders.size())
					{
						config.extruders[i].filament = text;
						changed = true;
					}
					break;
				case SimulatorPart::heaters:
					if (field == "name" && i < config.heaters.size())
					{
						config.heaters[i].name = text;
						changed = true;
					}
					break;
				case SimulatorPart::tools:
					if (field == "name" && i < config.tools.size())
					{
						config.tools[i].name = text;
						changed = true;
					}
					break;
				case SimulatorPart::fans:
					if (field == "name" && i < config.fans.size())
					{
						config.fans[i].name = text;
						changed = true;
					}
					break;
				case SimulatorPart::sensors:
					if (field == "name" && i < config.sensors.size())
					{
						config.sensors[i].name = text;
						changed = true;
					}
					break;
				case SimulatorPart::files:
					if (field == "name" && i < config.files.size())
					{
						config.files[i].name = text.empty() ? fmt::format("job {:d}.gcode", i + 1) : std::string(text);
						changed = true;
					}
					break;
				case SimulatorPart::axes:
				case SimulatorPart::spindles:
					break;
				}
			});
		update(); // shows what it is, if the text wasn't taken
		return changed;
	}

	bool SimulatorSource::setNumber(SimulatorItem item, std::string_view field, float value)
	{
		const size_t i = item.index;
		const int whole = static_cast<int>(std::lround(value));
		bool changed = false;
		edit(
			[&](Sim::Config& config)
			{
				switch (item.part)
				{
				case SimulatorPart::axes:
					if (i < config.axes.size() && (field == "min" || field == "max"))
					{
						(field == "min" ? config.axes[i].min : config.axes[i].max) = value;
						changed = true;
					}
					else if (const auto letters = Sim::axisLetters(); i < config.axes.size() && field == "letter" &&
																	  whole >= 0 &&
																	  whole < static_cast<int>(letters.size()))
					{
						// An axis that has the letter already takes this one's
						const std::string letter = letters[static_cast<size_t>(whole)];
						for (Sim::Axis& other : config.axes)
						{
							if (other.letter == letter)
							{
								other.letter = config.axes[i].letter;
							}
						}
						config.axes[i].letter = letter;
						changed = true;
					}
					break;
				case SimulatorPart::heaters:
					if (i < config.heaters.size())
					{
						Sim::Heater& heater = config.heaters[i];
						if (field == "role" && whole >= 0 && whole <= static_cast<int>(Sim::HeaterRole::other))
						{
							heater.role = static_cast<Sim::HeaterRole>(whole);
							changed = true;
						}
						else if (field == "max")
						{
							heater.max = value;
							changed = true;
						}
						else if (field == "heating_rate" || field == "cooling_rate")
						{
							(field == "heating_rate" ? heater.heatingRate : heater.coolingRate) = std::max(0.1f, value);
							changed = true;
						}
						else if (field == "fault")
						{
							heater.fault = whole != 0;
							changed = true;
						}
					}
					break;
				case SimulatorPart::extruders:
					break; // its filament is text
				case SimulatorPart::tools:
					if (i < config.tools.size())
					{
						Sim::Tool& tool = config.tools[i];
						if (field == "spindle" && whole >= 0 && whole <= static_cast<int>(config.spindles.size()))
						{
							tool.spindle = whole - 1;
							changed = true;
						}
					}
					break;
				case SimulatorPart::fans:
					if (i < config.fans.size() && (field == "tacho" || field == "thermostatic"))
					{
						(field == "tacho" ? config.fans[i].tacho : config.fans[i].thermostatic) = whole != 0;
						changed = true;
					}
					break;
				case SimulatorPart::sensors:
					if (i < config.sensors.size())
					{
						if (field == "kind" && whole >= 0 && whole <= static_cast<int>(Sim::SensorKind::input))
						{
							config.sensors[i].kind = static_cast<Sim::SensorKind>(whole);
							changed = true;
						}
						else if (field == "value")
						{
							config.sensors[i].value = value;
							changed = true;
						}
					}
					break;
				case SimulatorPart::spindles:
					if (i < config.spindles.size())
					{
						if (field == "max")
						{
							config.spindles[i].max = whole;
							changed = true;
						}
						else if (field == "reverse")
						{
							config.spindles[i].canReverse = whole != 0;
							changed = true;
						}
					}
					break;
				case SimulatorPart::files:
					if (i < config.files.size())
					{
						if (field == "print_time")
						{
							config.files[i].printTime = std::max(1, static_cast<int>(std::lround(value * 60.0f)));
							changed = true;
						}
						else if (field == "layers")
						{
							config.files[i].layers = std::max(1, whole);
							changed = true;
						}
						else if (field == "thumbnail")
						{
							config.files[i].thumbnail = whole != 0;
							changed = true;
						}
					}
					break;
				}
			});
		update();
		return changed;
	}

	bool SimulatorSource::toggle(SimulatorItem item, std::string_view field)
	{
		const auto flag = [&]() -> std::optional<int32_t>
		{
			switch (item.part)
			{
			case SimulatorPart::heaters:
				if (field == "fault")
				{
					return m_heaters[item.index].fault.get();
				}
				break;
			case SimulatorPart::files:
				if (field == "thumbnail")
				{
					return m_files[item.index].thumbnail.get();
				}
				break;
			case SimulatorPart::fans:
				if (field == "tacho" || field == "thermostatic")
				{
					const SimulatorFanRow& fan = m_fans[item.index];
					return field == "tacho" ? fan.tacho.get() : fan.thermostatic.get();
				}
				break;
			case SimulatorPart::spindles:
				if (field == "reverse")
				{
					return m_spindles[item.index].reverse.get();
				}
				break;
			default:
				break;
			}
			return std::nullopt;
		}();
		return flag && setNumber(item, field, *flag != 0 ? 0.0f : 1.0f);
	}

	void SimulatorSource::toggle(size_t tool, SimulatorItem choice)
	{
		const size_t t = tool;
		const int number = static_cast<int>(choice.index);
		edit(
			[&](Sim::Config& config)
			{
				if (t >= config.tools.size())
				{
					return;
				}
				std::vector<int>& numbers = choice.part == SimulatorPart::heaters	  ? config.tools[t].heaters
											: choice.part == SimulatorPart::extruders ? config.tools[t].extruders
																					  : config.tools[t].fans;
				if (std::ranges::contains(numbers, number))
				{
					std::erase(numbers, number);
				}
				else
				{
					numbers.push_back(number);
					std::ranges::sort(numbers);
				}
			});
		update();
	}

	std::optional<SimulatorNumber> SimulatorSource::number(SimulatorItem item, std::string_view field) const
	{
		const Sim::Config config = Sim::Simulator::get().config();
		const size_t i = item.index;
		switch (item.part)
		{
		case SimulatorPart::axes:
			if (i < config.axes.size() && (field == "min" || field == "max"))
			{
				return SimulatorNumber{.value = field == "min" ? config.axes[i].min : config.axes[i].max,
									   .min = -10000.0f,
									   .max = 10000.0f,
									   .unit = "mm"};
			}
			break;
		case SimulatorPart::heaters:
			if (i < config.heaters.size() && field == "max")
			{
				return SimulatorNumber{.value = config.heaters[i].max, .min = 0.0f, .max = 1000.0f, .unit = "°C"};
			}
			if (i < config.heaters.size() && (field == "heating_rate" || field == "cooling_rate"))
			{
				return SimulatorNumber{.value = field == "heating_rate" ? config.heaters[i].heatingRate
																		: config.heaters[i].coolingRate,
									   .min = 0.1f,
									   .max = 50.0f,
									   .unit = "°C/s"};
			}
			break;
		case SimulatorPart::sensors:
			if (i < config.sensors.size() && field == "value")
			{
				const Sim::Sensor& sensor = config.sensors[i];
				const bool temperature = sensor.kind == Sim::SensorKind::temperature;
				const bool probe = sensor.kind == Sim::SensorKind::probe;
				return SimulatorNumber{.value = sensor.value,
									   .min = temperature ? -50.0f : 0.0f,
									   .max = temperature ? 500.0f
											  : probe	  ? 1000.0f
														  : 1.0f,
									   .unit = std::string(sensorUnit(sensor.kind))};
			}
			break;
		case SimulatorPart::spindles:
			if (i < config.spindles.size() && field == "max")
			{
				return SimulatorNumber{.value = static_cast<float>(config.spindles[i].max),
									   .min = 100.0f,
									   .max = 100000.0f,
									   .unit = "rpm"};
			}
			break;
		case SimulatorPart::files:
			if (i < config.files.size() && field == "print_time")
			{
				return SimulatorNumber{.value = static_cast<float>(config.files[i].printTime) / 60.0f,
									   .min = 0.1f,
									   .max = 10000.0f,
									   .unit = "min"};
			}
			if (i < config.files.size() && field == "layers")
			{
				return SimulatorNumber{
					.value = static_cast<float>(config.files[i].layers), .min = 1.0f, .max = 10000.0f, .unit = ""};
			}
			break;
		default:
			break;
		}
		return std::nullopt;
	}
#else
	void SimulatorSource::add(SimulatorPart) {}

	void SimulatorSource::remove(SimulatorItem) {}

	bool SimulatorSource::setText(SimulatorItem, std::string_view, std::string_view)
	{
		return false;
	}

	bool SimulatorSource::setNumber(SimulatorItem, std::string_view, float)
	{
		return false;
	}

	bool SimulatorSource::toggle(SimulatorItem, std::string_view)
	{
		return false;
	}

	void SimulatorSource::toggle(size_t, SimulatorItem) {}

	std::optional<SimulatorNumber> SimulatorSource::number(SimulatorItem, std::string_view) const
	{
		return std::nullopt;
	}
#endif
} // namespace UI::Xml

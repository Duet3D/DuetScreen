/*
 * HeaterActions.cpp
 */

#include "HeaterActions.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/HeatSource.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <fmt/format.h>

namespace UI::HeaterActions
{
	/** "active" or "standby", translated */
	static std::string modeText(bool active)
	{
		return active ? _("temperature.status.active") : _("temperature.status.standby");
	}

	/**
	 * Cycle one of tool `tool`'s heaters (`slot` in its list) on: active, then standby, then off, as M568 A sets the
	 * tool's heaters; a heater with a fault is reset instead
	 */
	static void toggleToolHeater(size_t tool, size_t slot)
	{
		std::string gcode;
		{
			MODEL_LOCK();
			const auto heater = Xml::PrinterModel::toolHeater(tool, slot);
			if (!heater)
			{
				return;
			}
			const std::string state = Xml::PrinterModel::heaterState(*heater);
			if (state == "active")
			{
				gcode = fmt::format("M568 P{:d} A1\n", tool);
			}
			else if (state == "standby")
			{
				gcode = fmt::format("M568 P{:d} A0\n", tool);
			}
			else if (state == "off")
			{
				gcode = fmt::format("M568 P{:d} A2\n", tool);
			}
			else if (state == "fault")
			{
				gcode = fmt::format("M562 P{:d}\n", *heater);
			}
		}
		if (!gcode.empty())
		{
			Xml::Services::sendGcode(gcode);
		}
	}

	/**
	 * Cycle bed or chamber `number` on, as M140/M141 do: a bed goes active, standby (M144), off; a chamber has no
	 * standby, so goes active, off. A heater with a fault is reset instead.
	 */
	static void toggleBedOrChamber(bool bed, size_t number)
	{
		std::string gcode;
		{
			MODEL_LOCK();
			const auto places = bed ? Xml::PrinterModel::beds() : Xml::PrinterModel::chambers();
			const auto place =
				std::find_if(places.begin(),
							 places.end(),
							 [number](const Xml::PrinterModel::HeatedPlace& p) { return p.number == number; });
			if (place == places.end())
			{
				return;
			}
			const char* code = bed ? "M140" : "M141";
			const std::string state = Xml::PrinterModel::heaterState(place->heater);
			const auto active = Xml::PrinterModel::heater(place->heater)["active"].integer(0);
			if (state == "active" && bed)
			{
				gcode = fmt::format("M144 P{:d}\n", number);
			}
			else if (state == "active" || state == "standby")
			{
				gcode = fmt::format("{:s} P{:d} S-273.15\n", code, number);
			}
			else if (state == "off")
			{
				gcode = fmt::format("{:s} P{:d} S{:d}\n", code, number, active);
			}
			else if (state == "fault")
			{
				gcode = fmt::format("M562 P{:d}\n", place->heater);
			}
		}
		if (!gcode.empty())
		{
			Xml::Services::sendGcode(gcode);
		}
	}

	/** `actions.heater.toggle` */
	static void toggleHeater(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		if (const auto* heater = args.row<Xml::ToolHeaterRow>())
		{
			if (const auto* toolRow = args.row<Xml::ToolRow>())
			{
				toggleToolHeater(static_cast<size_t>(toolRow->number.get()), static_cast<size_t>(heater->index.get()));
			}
		}
		else if (const auto* bed = args.row<Xml::BedRow>())
		{
			toggleBedOrChamber(true, static_cast<size_t>(bed->number.get()));
		}
		else if (const auto* chamber = args.row<Xml::ChamberRow>())
		{
			toggleBedOrChamber(false, static_cast<size_t>(chamber->number.get()));
		}
		else
		{
			LOG_WARN("heaters.toggle must come from a tool heater, bed or chamber row");
		}
	}

	void setTemperature(Heated what, size_t number, size_t slot, bool active, std::optional<float> value)
	{
		ZoneScoped;
		Xml::NumberRequest request;
		{
			MODEL_LOCK();
			switch (what)
			{
			case Heated::ToolHeater:
			{
				const auto heater = Xml::PrinterModel::toolHeater(number, slot);
				if (!heater)
				{
					return;
				}
				request.header = _("temperature.numpad_header_tool_heater", number, slot, modeText(active));
				request.min = static_cast<float>(Xml::PrinterModel::heater(*heater)["min"].number());
				request.max = static_cast<float>(Xml::PrinterModel::heater(*heater)["max"].number());
				request.onConfirm = [number, slot, active](float v)
				{
					// M568 sets every heater of the tool: the others keep their targets
					std::string targets;
					{
						MODEL_LOCK();
						const OM::Node& tool = Xml::PrinterModel::tool(number);
						const OM::Node& temps = tool[active ? "active" : "standby"];
						for (size_t i = 0; i < tool["heaters"].size(); ++i)
						{
							targets += fmt::format("{:s}{:d}",
												   i > 0 ? ":" : "",
												   i == slot ? static_cast<int64_t>(v) : temps[i].integer(0));
						}
					}
					if (!targets.empty())
					{
						Xml::Services::sendGcode(
							fmt::format("M568 P{:d} {:c}{:s}\n", number, active ? 'S' : 'R', targets));
					}
				};
				break;
			}
			case Heated::Bed:
			case Heated::Chamber:
			{
				const bool bed = what == Heated::Bed;
				const auto places = bed ? Xml::PrinterModel::beds() : Xml::PrinterModel::chambers();
				const auto place =
					std::find_if(places.begin(),
								 places.end(),
								 [number](const Xml::PrinterModel::HeatedPlace& p) { return p.number == number; });
				if (place == places.end())
				{
					return;
				}
				request.header = _("temperature.numpad_header_bed_chamber",
								   bed ? _("temperature.bed") : _("temperature.chamber"),
								   slot,
								   modeText(active));
				request.min = static_cast<float>(Xml::PrinterModel::heater(place->heater)["min"].number());
				request.max = static_cast<float>(Xml::PrinterModel::heater(place->heater)["max"].number());
				request.onConfirm = [bed, number, active](float v)
				{
					Xml::Services::sendGcode(fmt::format("{:s} P{:d} {:s}{:d}\n",
														 bed ? "M140" : "M141",
														 number,
														 active ? "S" : "R",
														 static_cast<int32_t>(v)));
				};
				break;
			}
			}
		}

		if (value)
		{
			request.onConfirm(*value);
			return;
		}
		Xml::Services::askNumber(std::move(request));
	}

	/** `actions.heater.set_active` (`active`) and `actions.heater.set_standby`: set the edited value, or ask for one */
	static void setTemperature(const Xml::ActionArgs& args, bool active)
	{
		if (const auto* heaterRow = args.row<Xml::ToolHeaterRow>())
		{
			if (const auto* toolRow = args.row<Xml::ToolRow>())
			{
				setTemperature(Heated::ToolHeater,
							   static_cast<size_t>(toolRow->number.get()),
							   static_cast<size_t>(heaterRow->index.get()),
							   active,
							   args.value());
			}
		}
		else if (const auto* bedRow = args.row<Xml::BedRow>())
		{
			setTemperature(Heated::Bed,
						   static_cast<size_t>(bedRow->number.get()),
						   static_cast<size_t>(bedRow->index.get()),
						   active,
						   args.value());
		}
		else if (const auto* chamberRow = args.row<Xml::ChamberRow>())
		{
			setTemperature(Heated::Chamber,
						   static_cast<size_t>(chamberRow->number.get()),
						   static_cast<size_t>(chamberRow->index.get()),
						   active,
						   args.value());
		}
		else
		{
			LOG_WARN("actions.heater.set_active/standby must come from a tool heater, bed or chamber row");
		}
	}

	void registerAll()
	{
		Xml::Actions::addGlobal("heater.toggle",
								"Cycle the heater through active, standby and off. From a tool's `heaters` row, or a "
								"row of `beds` or `chambers`.",
								toggleHeater);
		Xml::Actions::addGlobal("heater.set_active",
								"Set the heater's active temperature to the edited value (`value_confirmed`), else ask "
								"for it with a number pad. From the same rows as `actions.heater.toggle`.",
								[](const Xml::ActionArgs& args) { setTemperature(args, true); });
		Xml::Actions::addGlobal("heater.set_standby",
								"As `actions.heater.set_active`, for the standby temperature",
								[](const Xml::ActionArgs& args) { setTemperature(args, false); });
	}
} // namespace UI::HeaterActions

/*
 * HeaterActions.cpp
 */

#include "HeaterActions.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Om/OmList.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <fmt/format.h>
#include <optional>
#include <variant>

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

	/** What a heater row is about, from its object model item's path */
	struct HeaterOf
	{
		Heated what;   // A tool's heater, a bed or a chamber
		size_t number; // The tool's, bed's or chamber's number
		size_t slot;   // For a tool's heater, its position among the tool's heaters; else the number
	};

	/**
	 * What heater `heater` heats: the bed or chamber it is the heater of, else the first tool it is a heater of;
	 * nothing if none uses it
	 */
	static std::optional<HeaterOf> userOf(size_t heater)
	{
		MODEL_LOCK();
		for (const bool bed : {true, false})
		{
			for (const auto& place : bed ? Xml::PrinterModel::beds() : Xml::PrinterModel::chambers())
			{
				if (place.heater == heater)
				{
					return HeaterOf{
						.what = bed ? Heated::Bed : Heated::Chamber, .number = place.number, .slot = place.number};
				}
			}
		}
		for (const size_t tool : Xml::PrinterModel::tools())
		{
			for (size_t slot = 0; slot < Xml::PrinterModel::tool(tool)["heaters"].size(); ++slot)
			{
				if (Xml::PrinterModel::toolHeater(tool, slot) == heater)
				{
					return HeaterOf{.what = Heated::ToolHeater, .number = tool, .slot = slot};
				}
			}
		}
		return std::nullopt;
	}

	/** The number of the heater `of` is about, if it has one. Call with the model locked. */
	static std::optional<size_t> heaterNumberOf(const HeaterOf& of)
	{
		if (of.what == Heated::ToolHeater)
		{
			return Xml::PrinterModel::toolHeater(of.number, of.slot);
		}
		for (const auto& place : of.what == Heated::Bed ? Xml::PrinterModel::beds() : Xml::PrinterModel::chambers())
		{
			if (place.number == of.number)
			{
				return place.heater;
			}
		}
		return std::nullopt;
	}

	/**
	 * The heater the object model row the action came from is about: an item of `tools[N].heaters` (tool N's heater
	 * at that slot), `heat.bedHeaters` (that bed), `heat.chamberHeaters` (that chamber), or `heat.heaters` (what that
	 * heater heats, see userOf()); nothing for anything else
	 */
	static std::optional<HeaterOf> heaterOf(const Xml::ActionArgs& args)
	{
		const auto* row = args.row<Xml::OmRow>();
		if (row == nullptr)
		{
			return std::nullopt;
		}
		const auto& segments = row->item().segments();
		const auto field = [&](size_t i) -> std::string_view
		{
			const auto* name = i < segments.size() ? std::get_if<std::string>(&segments[i]) : nullptr;
			return name ? std::string_view(*name) : std::string_view();
		};
		const auto index = [&](size_t i) -> std::optional<size_t>
		{
			const auto* number = i < segments.size() ? std::get_if<size_t>(&segments[i]) : nullptr;
			return number ? std::optional(*number) : std::nullopt;
		};
		if (segments.size() == 4 && field(0) == "tools" && index(1) && field(2) == "heaters" && index(3))
		{
			return HeaterOf{.what = Heated::ToolHeater, .number = *index(1), .slot = *index(3)};
		}
		if (segments.size() == 3 && field(0) == "heat" && index(2))
		{
			if (field(1) == "bedHeaters")
			{
				return HeaterOf{.what = Heated::Bed, .number = *index(2), .slot = *index(2)};
			}
			if (field(1) == "chamberHeaters")
			{
				return HeaterOf{.what = Heated::Chamber, .number = *index(2), .slot = *index(2)};
			}
			if (field(1) == "heaters")
			{
				return userOf(*index(2));
			}
		}
		return std::nullopt;
	}

	/** `actions.heater.toggle` */
	static void toggleHeater(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		const auto heater = heaterOf(args);
		if (!heater)
		{
			LOG_WARN("actions.heater.toggle must come from a row of a tool's heaters, the beds, the chambers or the "
					 "heaters");
			return;
		}
		if (heater->what == Heated::ToolHeater)
		{
			toggleToolHeater(heater->number, heater->slot);
		}
		else
		{
			toggleBedOrChamber(heater->what == Heated::Bed, heater->number);
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

	/**
	 * `actions.heater.set_active` (`active`) and `actions.heater.set_standby`: set the edited value, or ask for one.
	 * Without `active`, `actions.heater.set_target`: the standby temperature while the heater is on standby, else the
	 * active one.
	 */
	static void setTemperature(const Xml::ActionArgs& args, std::optional<bool> active)
	{
		const auto heater = heaterOf(args);
		if (!heater)
		{
			LOG_WARN("actions.heater.set_* must come from a row of a tool's heaters, the beds, the chambers or the "
					 "heaters");
			return;
		}
		if (!active)
		{
			MODEL_LOCK();
			const auto number = heaterNumberOf(*heater);
			active = !number || Xml::PrinterModel::heaterState(*number) != "standby";
		}
		setTemperature(heater->what, heater->number, heater->slot, *active, args.value());
	}

	void registerAll()
	{
		Xml::Actions::addGlobal(
			"heater.toggle",
			"Cycle the heater through active, standby and off. From a row of a tool's heaters "
			"(`om.tools[…].heaters`), of `om.heat.bedHeaters` or `om.heat.chamberHeaters`, or of "
			"`om.heat.heaters` (the bed or chamber it heats, else the first tool it is a heater of).",
			toggleHeater);
		Xml::Actions::addGlobal("heater.set_active",
								"Set the heater's active temperature to the edited value (`value_confirmed`), else ask "
								"for it with a number pad. From the same rows as `actions.heater.toggle`.",
								[](const Xml::ActionArgs& args) { setTemperature(args, true); });
		Xml::Actions::addGlobal("heater.set_standby",
								"As `actions.heater.set_active`, for the standby temperature",
								[](const Xml::ActionArgs& args) { setTemperature(args, false); });
		Xml::Actions::addGlobal("heater.set_target",
								"As `actions.heater.set_active`, for the target the heater heats to: its standby "
								"temperature while on standby, else its active one",
								[](const Xml::ActionArgs& args) { setTemperature(args, std::nullopt); });
	}
} // namespace UI::HeaterActions

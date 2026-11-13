/*
 * HeaterActions.cpp
 */

#include "HeaterActions.h"
#include "Debug.h"
#include "ObjectModel/BedOrChamber.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/HeatSource.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"

namespace UI::HeaterActions
{
	/** "active" or "standby", translated */
	static std::string modeText(bool active)
	{
		return active ? _("temperature.status.active") : _("temperature.status.standby");
	}

	/** `tools.toggle` */
	static void toggleTool(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		const auto* row = args.row<Xml::ToolRow>();
		if (row == nullptr)
		{
			LOG_WARN("tools.toggle must come from a row of the tools list");
			return;
		}
		if (auto tool = OM::GetTool(static_cast<size_t>(row->number.get())))
		{
			tool->ToggleState();
		}
	}

	/** `heaters.toggle` */
	static void toggleHeater(const Xml::ActionArgs& args)
	{
		ZoneScoped;
		if (const auto* heater = args.row<Xml::ToolHeaterRow>())
		{
			const auto* toolRow = args.row<Xml::ToolRow>();
			auto tool = toolRow ? OM::GetTool(static_cast<size_t>(toolRow->number.get())) : nullptr;
			if (tool)
			{
				tool->ToggleHeaterState(static_cast<size_t>(heater->index.get()));
			}
		}
		else if (const auto* bed = args.row<Xml::BedRow>())
		{
			if (auto item = OM::GetBed(static_cast<size_t>(bed->number.get())))
			{
				item->ToggleBedState();
			}
		}
		else if (const auto* chamber = args.row<Xml::ChamberRow>())
		{
			if (auto item = OM::GetChamber(static_cast<size_t>(chamber->number.get())))
			{
				item->ToggleChamberState();
			}
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
				auto tool = OM::GetTool(number);
				auto toolHeater = tool ? tool->GetHeater(slot) : nullptr;
				if (toolHeater == nullptr || toolHeater->heater == nullptr)
				{
					return;
				}
				request.header = _("temperature.numpad_header_tool_heater", number, slot, modeText(active));
				request.min = toolHeater->heater->min;
				request.max = toolHeater->heater->max;
				request.onConfirm = [number, slot, active](float v)
				{
					if (auto t = OM::GetTool(number))
					{
						t->SetHeaterTemps(slot, static_cast<int32_t>(v), active);
					}
				};
				break;
			}
			case Heated::Bed:
			case Heated::Chamber:
			{
				const bool bed = what == Heated::Bed;
				auto item = bed ? OM::GetBed(number) : OM::GetChamber(number);
				if (item == nullptr)
				{
					return;
				}
				request.header = _("temperature.numpad_header_bed_chamber",
								   bed ? _("temperature.bed") : _("temperature.chamber"),
								   slot,
								   modeText(active));
				request.min = item->GetMin();
				request.max = item->GetMax();
				request.onConfirm = [bed, number, active](float v)
				{
					auto i = bed ? OM::GetBed(number) : OM::GetChamber(number);
					if (i && bed)
					{
						i->SetBedTemp(static_cast<int32_t>(v), active);
					}
					else if (i)
					{
						i->SetChamberTemp(static_cast<int32_t>(v), active);
					}
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

	/** `heaters.set_active` (`active`) and `heaters.set_standby`: set the edited value, or ask for one */
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
			LOG_WARN("heaters.set_active/standby must come from a tool heater, bed or chamber row");
		}
	}

	void registerAll()
	{
		Xml::Actions::addGlobal(
			"tools.toggle", "Select the tool, or deselect it if selected. From a row of `tools`.", toggleTool);
		Xml::Actions::addGlobal("heaters.toggle",
								"Cycle the heater through active, standby and off. From a tool's `heaters` row, or a "
								"row of `beds` or `chambers`.",
								toggleHeater);
		Xml::Actions::addGlobal("heaters.set_active",
								"Set the heater's active temperature to the edited value (`value_confirmed`), else ask "
								"for it with a number pad. From the same rows as `heaters.toggle`.",
								[](const Xml::ActionArgs& args) { setTemperature(args, true); });
		Xml::Actions::addGlobal("heaters.set_standby",
								"As `heaters.set_active`, for the standby temperature",
								[](const Xml::ActionArgs& args) { setTemperature(args, false); });
	}
} // namespace UI::HeaterActions

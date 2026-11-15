/*
 * ToolSource.cpp
 */

#include "ToolSource.h"
#include "Debug.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <fmt/format.h>
#include <vector>

namespace UI::Xml
{
	namespace
	{
		/** The automatic style shows every tool equally up to this many heater rows: the tools' (a tool with none is
		 * one row), the beds' and the chambers' */
		constexpr size_t EQUAL_ROWS = 3;
	} // namespace

	void ToolTileRow::fields(FieldVisitor& f)
	{
		f.field("number", number, "The tool's number (actions.tool.show)");
		f.field("label", label, "E.g. \"T0\"");
		f.field("name", name, "Its name, else e.g. \"Tool 0\"");
		f.field(
			"state", state, "\"active\", \"standby\", \"off\" or \"fault\": its heaters' state, any fault marking it");
		f.field("in_use", inUse, "1 if it is the tool in use");
		f.field("shown", shown, "1 if it is the tool shown in detail");
	}

	ToolSource::ToolSource()
		: DataSource("tool")
	{
	}

	void ToolSource::fields(FieldVisitor& f)
	{
		f.field("current.shown", m_currentShown, "1 if there is a current tool with heaters, else 0");
		f.field("current.label", m_currentLabel, "The current tool, e.g. \"T0:\", or empty if it has no heaters");
		f.field("current.heaters",
				m_currentHeaters,
				"The current tool's heaters' current and target temperatures, e.g. \" 210°C/215°C 60°C/60°C\"");
		f.field("current.fault", m_currentFault, "1 if any of the current tool's heaters has a fault, else 0");
		f.field("shown",
				m_shown,
				"The tool the dashboard's tools widget shows in detail: the one picked (actions.tool.show), else the "
				"tool in use, else the first; -1 for none");
		f.field("following", m_following, "1 while the tool shown follows the tool in use, 0 once one is picked");
		f.field("summary_auto",
				m_summaryAuto,
				fmt::format("1 if the tools widget's automatic style shows one tool with the rest summarised: unless "
							"every tool is a print head (no spindle) and they, the beds and the chambers have {:d} "
							"heater rows or fewer, which it shows equally",
							EQUAL_ROWS));
		f.list("tiles",
			   m_tiles,
			   "A tile per tool, for the tools widget's summary (actions.tool.show); a tile's heaters are its tool's, "
			   "om.tools[{row.number}].heaters");
	}

	void ToolSource::onInit()
	{
		watchObjectModel({"tools", "heat", "state.currentTool", "sensors.analog"});
	}

	void ToolSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		showCurrentTool();
		showTools();
	}

	void ToolSource::clear()
	{
		MODEL_LOCK();
		showCurrentTool();
		showTools();
	}

	void ToolSource::show(std::optional<size_t> number)
	{
		MODEL_LOCK();
		// Picking the tool in use is following it again
		m_picked = number != PrinterModel::currentTool() ? number : std::nullopt;
		showTools();
	}

	void ToolSource::showTools()
	{
		const OM::Node& tools = PrinterModel::root()["tools"];
		// A picked tool is shown while it exists; else the tool in use, else the first
		if (m_picked && !tools[*m_picked].exists())
		{
			m_picked.reset();
		}
		const auto current = PrinterModel::currentTool();
		std::optional<size_t> shown = m_picked ? m_picked : current;
		std::vector<size_t> numbers;
		bool printHeads = true;
		size_t rows = 0;
		for (size_t number = 0; number < tools.size(); ++number)
		{
			const OM::Node& tool = tools[number];
			if (!tool.exists())
			{
				continue;
			}
			numbers.push_back(number);
			printHeads &= tool["spindle"].integer(-1) < 0 && tool["extruders"].size() > 0;
			rows += std::max<size_t>(1, tool["heaters"].size());
		}
		if (!shown && !numbers.empty())
		{
			shown = numbers.front();
		}
		m_shown.set(shown ? static_cast<int32_t>(*shown) : -1);
		m_following.set(m_picked ? 0 : 1);
		rows += PrinterModel::beds().size() + PrinterModel::chambers().size();
		m_summaryAuto.set(printHeads && rows <= EQUAL_ROWS ? 0 : 1);
		m_tiles.sync(numbers.size(),
					 [&](ToolTileRow& row, size_t i)
					 {
						 const size_t number = numbers[i];
						 const std::string_view name = tools[number]["name"].text();
						 row.number.set(static_cast<int32_t>(number));
						 row.label.set(fmt::format("T{:d}", number));
						 row.name.set(name.empty() ? _("temperature.default_tool_name", number) : std::string(name));
						 row.inUse.set(current == number ? 1 : 0);
						 row.shown.set(shown == number ? 1 : 0);
						 // Any fault marks the tool; else its state, as its heaters follow it
						 std::string state(tools[number]["state"].text());
						 const OM::Node& heaters = tools[number]["heaters"];
						 for (size_t slot = 0; slot < heaters.size(); ++slot)
						 {
							 if (const auto heater = PrinterModel::toolHeater(number, slot);
								 heater && PrinterModel::heaterState(*heater) == "fault")
							 {
								 state = "fault";
							 }
						 }
						 row.state.set(state.empty() ? std::string("off") : state);
					 });
	}

	void ToolSource::showCurrentTool()
	{
		const auto current = PrinterModel::currentTool();
		const OM::Node& heaterNumbers = current ? PrinterModel::tool(*current)["heaters"] : OM::Node::missing();
		if (heaterNumbers.size() == 0)
		{
			m_currentShown.set(0);
			m_currentLabel.set("");
			m_currentHeaters.set("");
			m_currentFault.set(0);
			return;
		}
		std::string heaters;
		bool fault = false;
		for (size_t slot = 0; slot < heaterNumbers.size(); ++slot)
		{
			if (const auto number = PrinterModel::toolHeater(*current, slot))
			{
				heaters += fmt::format(" {:g}°C/{:d}°C",
									   static_cast<float>(PrinterModel::heater(*number)["current"].number()),
									   PrinterModel::heaterTarget(*number));
				fault |= PrinterModel::heaterState(*number) == "fault";
			}
		}
		m_currentShown.set(1);
		m_currentLabel.set(fmt::format("T{:d}:", *current));
		m_currentHeaters.set(heaters);
		m_currentFault.set(fault ? 1 : 0);
	}
} // namespace UI::Xml

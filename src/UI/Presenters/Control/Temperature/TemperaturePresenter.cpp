/*
 * TemperaturePresenter.cpp
 */

#include "TemperaturePresenter.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "i18n/i18n.h"
#include <fmt/format.h>
#include <vector>

namespace UI
{
	void TemperatureToolRow::fields(Xml::FieldVisitor& f)
	{
		f.field("number", number, "Tool number, as used by T; the tool's own fields are om.tools[{row.number}]");
		f.field("filament_shown", filamentShown, "1 if the tool takes filament");
		f.field("filament_text", filamentText, "The filament loaded, or \"No Filament Loaded\"");
		f.field("filament_loaded", filamentLoaded, "1 if filament is loaded, so it can be unloaded");
	}

	void TemperatureToolRow::update(size_t toolNumber)
	{
		number.set(static_cast<int32_t>(toolNumber));
		filamentShown.set(Xml::PrinterModel::takesFilament(toolNumber) ? 1 : 0);
		const std::string_view loaded = Xml::PrinterModel::filament(toolNumber);
		filamentText.set(loaded.empty() ? std::string_view(_("filament.not_loaded")) : loaded);
		filamentLoaded.set(loaded.empty() ? 0 : 1);
	}

	TemperaturePresenter::TemperaturePresenter()
		: XmlPresenter("temperature")
	{
	}

	void TemperaturePresenter::fields(Xml::FieldVisitor& f)
	{
		f.list("tools", m_tools, "The tools with heaters");
		f.field("selected_tool", m_selectedTool, "E.g. \"Selected Tool: Tool 0\"");
	}

	void TemperaturePresenter::onInit()
	{
		m_watch = Xml::OmWatch({"tools", "heat", "state.currentTool", "sensors.analog", "move.extruders"},
							   [this] { showTools(); });
	}

	void TemperaturePresenter::onActivate()
	{
		showTools();
	}

	void TemperaturePresenter::onDisconnect()
	{
		m_tools.clear();
		m_selectedTool.set(_("temperature.selected_tool_label", ""));
	}

	void TemperaturePresenter::showTools()
	{
		ZoneScoped;
		MODEL_LOCK();
		std::vector<size_t> heated;
		for (const size_t number : Xml::PrinterModel::tools())
		{
			if (Xml::PrinterModel::tool(number)["heaters"].size() > 0)
			{
				heated.push_back(number);
			}
		}
		m_tools.sync(heated.size(), [&heated](TemperatureToolRow& row, size_t i) { row.update(heated[i]); });

		const auto current = Xml::PrinterModel::currentTool();
		m_selectedTool.set(
			_("temperature.selected_tool_label", current ? Xml::PrinterModel::toolName(*current) : std::string()));
	}
} // namespace UI

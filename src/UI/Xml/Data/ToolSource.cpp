/*
 * ToolSource.cpp
 */

#include "ToolSource.h"
#include "Debug.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"

namespace UI::Xml
{
	void ToolRow::fields(FieldVisitor& f)
	{
		f.field("number", number, "Tool number, as used by T");
		f.field("name", name, "Display name");
		f.field("selected", selected, "1 if this is the active tool, else 0");
		f.field("heater_count", heaterCount, "Number of heaters");
		f.list("heaters", heaters, "One row per heater of the tool");
	}

	void ToolRow::update(OM::Tool& tool)
	{
		number.set(static_cast<int32_t>(tool.index));
		name.set(tool.GetName());
		selected.set(tool.status == OM::ToolStatus::active ? 1 : 0);
		heaterCount.set(static_cast<int32_t>(tool.GetHeaterCount()));

		heaters.sync(tool.GetHeaterCount(),
					 [&tool](ToolHeaterRow& row, size_t i)
					 {
						 auto toolHeater = tool.GetHeater(i);
						 if (toolHeater == nullptr || toolHeater->heater == nullptr)
						 {
							 return;
						 }
						 const auto& heater = *toolHeater->heater;
						 const std::string_view sensorName = heater.GetName();
						 const std::string name =
							 sensorName.empty() ? _("temperature.heater", heater.index) : std::string(sensorName);
						 row.update({.heater = static_cast<int32_t>(heater.index),
									 .name = name,
									 .state = heater.GetHeaterStatusStr(),
									 .current = heater.current,
									 .active = toolHeater->activeTemp,
									 .standby = toolHeater->standbyTemp,
									 .min = heater.min,
									 .max = heater.max});
					 });
	}

	ToolSource::ToolSource()
		: DataSource("tools")
	{
	}

	void ToolSource::fields(FieldVisitor& f)
	{
		f.list("tools", m_tools, "One row per tool, in slot order");
		f.field("current_tool.shown", m_currentShown, "1 if there is a current tool with heaters, else 0");
		f.field("current_tool.label", m_currentLabel, "The current tool, e.g. \"T0:\", or empty if it has no heaters");
		f.field("current_tool.heaters",
				m_currentHeaters,
				"The current tool's heaters' current and target temperatures, e.g. \" 210°C/215°C 60°C/60°C\"");
		f.field("current_tool.fault", m_currentFault, "1 if any of the current tool's heaters has a fault, else 0");
	}

	void ToolSource::onInit()
	{
		registerEventListener<EventType::ToolData>(this, &ToolSource::update);
		registerEventListener<EventType::HeaterData>(this, &ToolSource::update);
	}

	void ToolSource::update()
	{
		ZoneScoped;
		MODEL_LOCK();
		m_tools.sync(OM::GetToolCount(),
					 [](ToolRow& row, size_t slot)
					 {
						 if (auto tool = OM::GetToolBySlot(slot))
						 {
							 row.update(*tool);
						 }
					 });
		showCurrentTool();
	}

	void ToolSource::clear()
	{
		m_tools.clear();
		MODEL_LOCK();
		showCurrentTool();
	}

	void ToolSource::showCurrentTool()
	{
		const auto tool = OM::GetCurrentTool();
		if (!tool || tool->GetHeaterCount() == 0)
		{
			m_currentShown.set(0);
			m_currentLabel.set("");
			m_currentHeaters.set("");
			m_currentFault.set(0);
			return;
		}
		std::string heaters;
		bool fault = false;
		tool->IterateHeaters(
			[&](OM::ToolHeaterPtr toolHeater, size_t)
			{
				if (const auto heater = toolHeater->heater)
				{
					heaters += fmt::format(" {:g}°C/{:d}°C", heater->current, heater->GetTargetTemp());
					fault |= heater->status == OM::Heat::HeaterStatus::fault;
				}
			});
		m_currentShown.set(1);
		m_currentLabel.set(fmt::format("T{:d}:", tool->index));
		m_currentHeaters.set(heaters);
		m_currentFault.set(fault ? 1 : 0);
	}
} // namespace UI::Xml

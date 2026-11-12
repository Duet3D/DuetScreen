/*
 * ToolSource.cpp
 */

#include "ToolSource.h"
#include "Debug.h"
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
		f.list("heaters", heaters, "One row per heater of the tool");
	}

	void ToolRow::update(OM::Tool& tool)
	{
		number.set(static_cast<int32_t>(tool.index));
		name.set(tool.GetName());
		selected.set(tool.status == OM::ToolStatus::active ? 1 : 0);

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
									 .standby = toolHeater->standbyTemp});
					 });
	}

	ToolSource::ToolSource()
		: DataSource("tools")
	{
	}

	void ToolSource::fields(FieldVisitor& f)
	{
		f.list("tools", m_tools, "One row per tool, in slot order");
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
	}

	void ToolSource::clear()
	{
		m_tools.clear();
	}
} // namespace UI::Xml

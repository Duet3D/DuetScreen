/*
 * ToolSource.cpp
 */

#include "ToolSource.h"
#include "Debug.h"
#include "PrinterModel.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"

namespace UI::Xml
{
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
	}

	void ToolSource::clear()
	{
		MODEL_LOCK();
		showCurrentTool();
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

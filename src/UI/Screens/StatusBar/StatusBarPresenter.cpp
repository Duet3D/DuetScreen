/*
 * StatusBarPresenter.cpp
 */

#include "StatusBarPresenter.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"

namespace UI
{
	StatusBarPresenter::StatusBarPresenter()
		: XmlPresenter("status_bar")
	{
	}

	void StatusBarPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("status", m_status, "Connection and state, e.g. \"USB - Idle\"");
		f.field("name", m_name, "The printer's name");
		f.field("has_tool", m_hasTool, "1 if there is a current tool with heaters, else 0");
		f.field("tool", m_tool, "The current tool, e.g. \"T0:\", or empty if it has no heaters");
		f.field("heaters", m_heaters, "The current tool's heaters' current and target temperatures");
		f.field("fault", m_fault, "1 if any of the current tool's heaters has a fault, else 0");
	}

	void StatusBarPresenter::onInit()
	{
		registerEventListener<EventType::NetworkName>(this, &StatusBarPresenter::updateName);
		registerEventListener<EventType::Status>(this, &StatusBarPresenter::updateStatus);
		registerEventListener<EventType::HeaterData>(this, &StatusBarPresenter::updateHeaters);
	}

	void StatusBarPresenter::onActivate()
	{
		ZoneScoped;
		updateName();
		updateStatus(OM::GetStatus());
		updateHeaters();
	}

	void StatusBarPresenter::onConnect()
	{
		showStatus("connected");
	}

	void StatusBarPresenter::onDisconnect()
	{
		showStatus("disconnected");
		m_name.set("");
		updateHeaters();
	}

	void StatusBarPresenter::updateName()
	{
		m_name.set(OM::GetPrinterName());
	}

	void StatusBarPresenter::updateStatus(const OM::PrinterStatus& /* status */)
	{
		showStatus(OM::GetStatusText());
	}

	void StatusBarPresenter::showStatus(std::string_view state)
	{
		m_status.set(
			fmt::format("{:s} - {:s}", _(Comm::DUET.GetCommunicationTypeName()), _(fmt::format("state.{:s}", state))));
	}

	void StatusBarPresenter::updateHeaters()
	{
		ZoneScoped;
		MODEL_LOCK();
		const auto tool = OM::GetCurrentTool();
		if (!tool || tool->GetHeaterCount() == 0)
		{
			m_hasTool.set(0);
			m_tool.set("");
			m_heaters.set("");
			m_fault.set(0);
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

		m_hasTool.set(1);
		m_tool.set(fmt::format("T{:d}:", tool->index));
		m_heaters.set(heaters);
		m_fault.set(fault ? 1 : 0);
	}
} // namespace UI

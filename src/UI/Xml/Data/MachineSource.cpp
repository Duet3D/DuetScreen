/*
 * MachineSource.cpp
 */

#include "MachineSource.h"
#include "Hardware/Duet.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI::Xml
{
	MachineSource::MachineSource()
		: DataSource("machine")
	{
	}

	void MachineSource::fields(FieldVisitor& f)
	{
		f.field("machine.status", m_status, "State key, e.g. idle, processing, paused; translated under state.*");
		f.field("machine.printing",
				m_printing,
				"1 while a job is running and not paused, else 0: controls that would change the job are disabled");
		f.field("machine.job_active",
				m_jobActive,
				"1 while there is a job (running, pausing, paused, resuming or simulating), else 0");
		f.field("machine.connection", m_connection, "How the screen is connected to the printer, e.g. \"USB\"");
		f.field("machine.state", m_state, "The printer's state, translated, e.g. \"Idle\"");
		f.field("machine.summary", m_summary, "The connection and the state, e.g. \"USB - Idle\"");
	}

	void MachineSource::onInit()
	{
		registerEventListener<EventType::Status>(this, &MachineSource::statusChanged);
	}

	void MachineSource::update()
	{
		statusChanged(OM::GetStatus());
	}

	void MachineSource::onConnect()
	{
		update();
		showState("connected");
	}

	void MachineSource::clear()
	{
		statusChanged(OM::GetStatus());
		showState("disconnected");
	}

	void MachineSource::statusChanged(const OM::PrinterStatus& status)
	{
		m_status.set(OM::GetStatusText());
		m_printing.set(OM::IsPrintingStatus(status) && status != OM::PrinterStatus::paused ? 1 : 0);
		m_jobActive.set(OM::IsPrintingStatus(status) ? 1 : 0);
		showState(OM::GetStatusText());
	}

	void MachineSource::showState(std::string_view state)
	{
		m_connection.set(_(Comm::DUET.GetCommunicationTypeName()));
		m_state.set(_(fmt::format("state.{:s}", state)));
		m_summary.set(fmt::format("{:s} - {:s}", m_connection.get(), m_state.get()));
	}
} // namespace UI::Xml

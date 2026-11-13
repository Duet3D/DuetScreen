/*
 * MachineSource.cpp
 */

#include "MachineSource.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Model.h"

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
	}

	void MachineSource::onInit()
	{
		registerEventListener<EventType::Status>(this, &MachineSource::statusChanged);
	}

	void MachineSource::update()
	{
		statusChanged(OM::GetStatus());
	}

	void MachineSource::statusChanged(const OM::PrinterStatus& status)
	{
		m_status.set(OM::GetStatusText());
		m_printing.set(OM::IsPrintingStatus(status) && status != OM::PrinterStatus::paused ? 1 : 0);
		m_jobActive.set(OM::IsPrintingStatus(status) ? 1 : 0);
	}

	void MachineSource::clear()
	{
		update();
	}
} // namespace UI::Xml

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
		f.field("machine.status", m_status, "State key, e.g. idle, processing, paused");
		f.field("machine.printing", m_printing, "1 while a job is running and not paused, else 0");
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
	}

	void MachineSource::clear()
	{
		update();
	}
} // namespace UI::Xml

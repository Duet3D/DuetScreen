/*
 * MachineSource.h
 *
 * Publishes the printer's state.
 *
 *   machine.status      state key, e.g. idle, processing, paused (translations are under state.*)
 *   machine.printing    1 while a job is running and not paused, else 0: controls that would change the job (heater
 *                       states, tool selection) are disabled
 *   machine.job_active  1 while there is a job (running, pausing, paused, resuming or simulating), else 0: the job
 *                       list is disabled, so the job can't be replaced
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"

namespace OM
{
	enum class PrinterStatus;
}

namespace UI::Xml
{
	class MachineSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		MachineSource();

		/** Declare `machine.status`, `machine.printing` and `machine.job_active` */
		void fields(FieldVisitor& f) override;

		/** Read the printer's state */
		void update() override;

		/** Forget the printer's state */
		void clear() override;

	  protected:
		/** Listen for state changes */
		void onInit() override;

	  private:
		/** The printer's state changed */
		void statusChanged(const OM::PrinterStatus& status);

		StringSubject<16> m_status; // State key
		IntSubject m_printing;		// 1 while a job is running and not paused
		IntSubject m_jobActive;		// 1 while there is a job, even paused
	};
} // namespace UI::Xml

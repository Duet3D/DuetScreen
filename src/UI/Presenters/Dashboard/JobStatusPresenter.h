/*
 * JobStatusPresenter.h
 *
 * Behaviour for the job status panel (assets/ui/components/dashboard/dashboard_job_status.xml): its babystepping
 * dialog. Everything it shows of the printer (the job, its progress, times, thumbnail and controls, positions, speeds
 * and flow) is global data any view can show: `job.*` and `move.*` (UI/Xml/Data/JobSource.h, MoveSource.h). Pausing,
 * resuming and printing again are global actions (JobActions.h), and so is asking whether to cancel (the shared
 * question, Modals/JobCancelPresenter.h).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class JobStatusPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		JobStatusPresenter();

		/** Declare the panel's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Close the dialog: the job may have changed */
		void onDeactivate() override;

	  private:
		Xml::IntSubject m_babystepOpen; // 1 while the babystepping dialog is open
	};
} // namespace UI

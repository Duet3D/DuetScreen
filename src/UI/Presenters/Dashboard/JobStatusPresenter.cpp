/*
 * JobStatusPresenter.cpp
 */

#include "JobStatusPresenter.h"

namespace UI
{
	JobStatusPresenter::JobStatusPresenter()
		: XmlPresenter("dashboard_job_status")
	{
		action("job.babystep",
			   "Open the babystepping dialog (sets `babystep_open`)",
			   [this](const Xml::ActionArgs&) { m_babystepOpen.set(1); });
		action("job.close_babystep",
			   "Close the babystepping dialog",
			   [this](const Xml::ActionArgs&) { m_babystepOpen.set(0); });
	}

	void JobStatusPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("babystep_open", m_babystepOpen, "1 while the babystepping dialog is open");
	}

	void JobStatusPresenter::onDeactivate()
	{
		m_babystepOpen.set(0);
	}
} // namespace UI

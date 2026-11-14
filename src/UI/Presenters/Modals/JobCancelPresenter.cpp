/*
 * JobCancelPresenter.cpp
 */

#include "JobCancelPresenter.h"
#include "Debug.h"
#include "ObjectModel/Files.h"
#include "UI/Core/Navigation.h"
#include "UI/Xml/View/XmlView.h"

namespace UI
{
	JobCancelPresenter* JobCancelPresenter::shared()
	{
		auto* view = findView<Xml::XmlView>("job_cancel");
		auto* presenter = view != nullptr ? dynamic_cast<JobCancelPresenter*>(view->getPresenter()) : nullptr;
		if (presenter == nullptr)
		{
			LOG_WARN("There is no question before cancelling (a view called 'job_cancel' showing job_cancel.xml)");
		}
		return presenter;
	}

	JobCancelPresenter::JobCancelPresenter()
		: XmlPresenter("job_cancel")
	{
		action("job.cancel",
			   "Cancel the job, which also turns the heaters off, and close the question",
			   [this](const Xml::ActionArgs&)
			   {
				   m_confirm.set(0);
				   OM::FileSystem::StopPrint(); // Also turns the heaters off
			   });
		action("job.close_cancel",
			   "Close the question without cancelling the job",
			   [this](const Xml::ActionArgs&) { m_confirm.set(0); });
	}

	void JobCancelPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("confirm", m_confirm, "1 while asking whether to cancel the job");
	}

	void JobCancelPresenter::ask()
	{
		m_confirm.set(1);
	}
} // namespace UI

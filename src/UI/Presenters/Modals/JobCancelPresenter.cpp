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
	namespace
	{
		/** The shared question's presenter: the view called `job_cancel`'s; nullptr if there is none */
		JobCancelPresenter* sharedQuestion()
		{
			auto* view = findView<Xml::XmlView>("job_cancel");
			auto* presenter = view != nullptr ? dynamic_cast<JobCancelPresenter*>(view->getPresenter()) : nullptr;
			if (presenter == nullptr)
			{
				LOG_WARN("There is no question before cancelling (a view called 'job_cancel' showing job_cancel.xml)");
			}
			return presenter;
		}
	} // namespace

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

	void JobCancelPresenter::registerActions()
	{
		Xml::Actions::addGlobal("job.ask_cancel",
								"Ask whether to cancel the job; it is cancelled, which also turns the heaters off, "
								"only if the user says so",
								[](const Xml::ActionArgs&)
								{
									if (auto* question = sharedQuestion())
									{
										question->ask();
									}
								});
	}

	void JobCancelPresenter::ask()
	{
		m_confirm.set(1);
	}
} // namespace UI

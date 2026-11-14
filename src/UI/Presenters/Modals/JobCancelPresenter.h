/*
 * JobCancelPresenter.h
 *
 * Behaviour for the shared question before cancelling the job (assets/ui/components/modals/job_cancel.xml), which
 * any view asks with the global action `actions.job.ask_cancel` (Dashboard/JobActions.cpp). Cancelling also turns the heaters off, so
 * the job is never cancelled without asking.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class JobCancelPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with the question closed */
		JobCancelPresenter();

		/** Declare the question's field */
		void fields(Xml::FieldVisitor& f) override;

		/** The shared question's presenter: the view called `job_cancel`'s; nullptr, with a warning, if there is none */
		static JobCancelPresenter* shared();

		/** Ask whether to cancel the job */
		void ask();

	  private:
		Xml::IntSubject m_confirm; // 1 while asking
	};
} // namespace UI

/*
 * JobCancelPresenter.h
 *
 * Behaviour for the shared question before cancelling the job (assets/ui/components/modals/job_cancel.xml), which
 * any view asks with the global action `job.ask_cancel` (registerActions()). Cancelling also turns the heaters off, so
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

		/** Register `job.ask_cancel` as a global action, for every view */
		static void registerActions();

		/** Ask whether to cancel the job */
		void ask();

	  private:
		Xml::IntSubject m_confirm; // 1 while asking
	};
} // namespace UI

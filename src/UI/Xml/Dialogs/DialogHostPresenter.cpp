/*
 * DialogHostPresenter.cpp
 */

#include "DialogHostPresenter.h"
#include "DialogHost.h"
#include <string>

namespace UI::Xml
{
	DialogHostPresenter::DialogHostPresenter()
		: XmlPresenter("dialog_host")
	{
		action("dialog.back",
			   "Go back to the step before, as the back button does",
			   [this](const ActionArgs&)
			   {
				   if (m_host != nullptr)
				   {
					   m_host->goBack();
				   }
			   });
		action("dialog.close",
			   "Close the whole dialog, as the close button does, unless a step of it is blocking",
			   [this](const ActionArgs&)
			   {
				   if (m_host != nullptr)
				   {
					   m_host->closeShown();
				   }
			   });
		action("dialog.outside",
			   "The backdrop was tapped: close the whole dialog, unless a step of it is blocking",
			   [this](const ActionArgs&)
			   {
				   if (m_host != nullptr)
				   {
					   m_host->tappedOutside();
				   }
			   });
	}

	DialogHostPresenter::~DialogHostPresenter() = default;

	void DialogHostPresenter::fields(FieldVisitor& f)
	{
		f.field("title", m_title, "The title of the step shown");
		f.field("from", m_from, "The title of the step it came from; empty on a dialog's first step");
		f.field("can_go_back", m_canGoBack, "1 if the back button shows: a later step that isn't blocking");
		f.field("can_close", m_canClose, "1 if the close button shows: no step of the dialog is blocking");
	}

	void DialogHostPresenter::show(lv_subject_t* title, lv_subject_t* from, bool canGoBack, bool canClose)
	{
		m_titleMirror.follow(title);
		m_fromMirror.follow(from);
		m_canGoBack.set(canGoBack ? 1 : 0);
		m_canClose.set(canClose ? 1 : 0);
	}

	void DialogHostPresenter::Mirror::follow(lv_subject_t* source)
	{
		if (m_observer != nullptr)
		{
			lv_observer_remove(m_observer);
			m_observer = nullptr;
		}
		if (source == nullptr)
		{
			m_copy.set("");
			return;
		}
		// Adding the observer calls it at once, which copies the title as it is now
		m_observer = lv_subject_add_observer(source, changedCb, this);
	}

	void DialogHostPresenter::Mirror::changedCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* mirror = static_cast<Mirror*>(lv_observer_get_user_data(observer));
		switch (subject->type)
		{
		case LV_SUBJECT_TYPE_STRING:
		case LV_SUBJECT_TYPE_POINTER:
			mirror->m_copy.set(lv_subject_get_string(subject));
			break;
		case LV_SUBJECT_TYPE_INT:
			mirror->m_copy.set(std::to_string(lv_subject_get_int(subject)));
			break;
		default:
			mirror->m_copy.set("");
			break;
		}
	}
} // namespace UI::Xml

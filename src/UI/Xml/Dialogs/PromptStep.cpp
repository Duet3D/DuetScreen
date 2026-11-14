/*
 * PromptStep.cpp
 */

#include "PromptStep.h"
#include "DialogHost.h"
#include "UI/Xml/View/XmlView.h"

namespace UI::Xml
{
	PromptStep::PromptStep(DialogHost& host,
						   const std::string& component,
						   std::shared_ptr<XmlPresenter> presenter,
						   const std::string& title,
						   Size size,
						   lv_obj_t* asker)
		: m_host(host)
		, m_title(title)
		, m_size(std::move(size))
		, m_asker(asker)
	{
		m_view = std::make_unique<XmlView>(component, host.parking(), component, std::move(presenter));
		// Hidden until the surface shows it, which shows it as a view
		m_view->hide();
		if (m_asker != nullptr)
		{
			lv_obj_add_event_cb(m_asker, askerDeletedCb, LV_EVENT_DELETE, this);
		}
	}

	PromptStep::~PromptStep()
	{
		if (m_asker != nullptr)
		{
			lv_obj_remove_event_cb_with_user_data(m_asker, askerDeletedCb, this);
		}
	}

	lv_obj_t* PromptStep::content()
	{
		return m_view->getRootPtr();
	}

	lv_obj_t* PromptStep::home()
	{
		return m_host.parking();
	}

	lv_subject_t* PromptStep::title()
	{
		return m_title.raw();
	}

	void PromptStep::dismissed()
	{
		if (!m_answered)
		{
			m_answered = true;
			if (m_onDismissed)
			{
				m_onDismissed();
			}
		}
	}

	void PromptStep::finish(const std::function<void()>& handBack)
	{
		if (m_answered)
		{
			return;
		}
		m_answered = true;
		// Taken before the step goes: it is deleted once this event is over, and the answer may open another prompt,
		// which must come after this one has gone
		const auto answer = handBack;
		m_host.finish(*this);
		if (answer)
		{
			answer();
		}
	}

	void PromptStep::askerDeletedCb(lv_event_t* e)
	{
		auto* step = static_cast<PromptStep*>(lv_event_get_user_data(e));
		step->m_answered = true;
		step->m_asker = nullptr;
		step->m_host.finish(*step);
	}
} // namespace UI::Xml

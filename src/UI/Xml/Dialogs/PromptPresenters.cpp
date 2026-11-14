/*
 * PromptPresenters.cpp
 */

#include "PromptPresenters.h"
#include "PromptStep.h"
#include "i18n/i18n.h"
#include "utils/utils.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>

namespace UI::Xml
{
	/* ---------- NumberPrompt ---------- */

	NumberPrompt::NumberPrompt()
		: XmlPresenter("dialog_number")
		, m_pad(*this, "number")
	{
		action("number.confirm",
			   "Hand back what is typed, if it can be, and go back",
			   [this](const ActionArgs&) { confirm(); });
		action("number.cancel",
			   "Go back without handing anything back, as the back button does",
			   [this](const ActionArgs&)
			   {
				   if (m_step != nullptr)
				   {
					   m_step->cancel();
				   }
			   });
	}

	void NumberPrompt::fields(FieldVisitor& f)
	{
		m_pad.fields(f);
	}

	void NumberPrompt::ask(Prompts::NumberRequest request, PromptStep& step)
	{
		m_step = &step;
		m_text.reset();
		m_pad.askNumber(request.min, request.max, request.value);
		m_number = std::move(request);
	}

	void NumberPrompt::ask(Prompts::TextRequest request, PromptStep& step)
	{
		m_step = &step;
		m_number.reset();
		m_pad.askText(request.text);
		m_text = std::move(request);
	}

	void NumberPrompt::confirm()
	{
		if (m_step == nullptr || !m_pad.confirmable())
		{
			return;
		}
		if (m_number)
		{
			m_step->finish(
				[onConfirm = m_number->onConfirm, value = m_pad.value()]()
				{
					if (onConfirm)
					{
						onConfirm(value);
					}
				});
		}
		else if (m_text)
		{
			m_step->finish(
				[onConfirm = m_text->onConfirm, typed = std::string(m_pad.text())]()
				{
					if (onConfirm)
					{
						onConfirm(typed);
					}
				});
		}
	}

	/* ---------- ChoicePrompt ---------- */

	void ChoiceRow::fields(FieldVisitor& f)
	{
		f.field("value", value, "The value handed back if it is chosen");
		f.field("title", title, "What the user sees");
		f.field("chosen", chosen, "1 for the value chosen now");
	}

	ChoicePrompt::ChoicePrompt()
		: XmlPresenter("dialog_choice")
	{
		action("choice.choose",
			   "Hand back the value of the row and go back. From a row of `choices`.",
			   [this](const ActionArgs& args)
			   {
				   const auto* row = args.row<ChoiceRow>();
				   if (row == nullptr || m_step == nullptr)
				   {
					   return;
				   }
				   m_step->finish(
					   [onChoose = m_onChoose, value = std::string(row->value.get())]()
					   {
						   if (onChoose)
						   {
							   onChoose(value);
						   }
					   });
			   });
	}

	void ChoicePrompt::fields(FieldVisitor& f)
	{
		f.list("choices", m_choices, "The values to choose from, in order");
	}

	void ChoicePrompt::ask(Prompts::ChoiceRequest request, PromptStep& step)
	{
		m_step = &step;
		m_onChoose = std::move(request.onChoose);
		const auto& choices = request.choices;
		m_choices.sync(choices.size(),
					   [&](ChoiceRow& row, size_t i)
					   {
						   row.value.set(choices[i].value);
						   row.title.set(choices[i].title);
						   row.chosen.set(choices[i].value == request.chosen ? 1 : 0);
					   });
	}

	/* ---------- QuestionPrompt ---------- */

	QuestionPrompt::QuestionPrompt()
		: XmlPresenter("dialog_question")
		, m_yes(_("common.yes"))
		, m_no(_("common.no"))
	{
		action("question.yes",
			   "Agree, and go back",
			   [this](const ActionArgs&)
			   {
				   if (m_step != nullptr)
				   {
					   m_step->finish(m_onYes);
				   }
			   });
		action("question.no",
			   "Don't agree, and go back",
			   [this](const ActionArgs&)
			   {
				   if (m_step != nullptr)
				   {
					   m_step->finish(m_onNo);
				   }
			   });
	}

	void QuestionPrompt::fields(FieldVisitor& f)
	{
		f.field("text", m_text, "The question");
		f.field("yes", m_yes, "The label of the button that agrees");
		f.field("no", m_no, "The label of the button that doesn't");
	}

	void QuestionPrompt::ask(Prompts::QuestionRequest request, PromptStep& step)
	{
		m_step = &step;
		m_onYes = std::move(request.onYes);
		m_onNo = std::move(request.onNo);
		m_text.set(request.text);
		m_yes.set(request.yes.empty() ? _("common.yes") : request.yes);
		m_no.set(request.no.empty() ? _("common.no") : request.no);
	}
} // namespace UI::Xml

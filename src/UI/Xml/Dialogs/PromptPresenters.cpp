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

	void NumberPresetRow::fields(FieldVisitor& f)
	{
		f.field("title", title, "What it is for, e.g. \"PLA\"");
		f.field("value", value, "The value with its unit, e.g. \"210 °C\"");
		f.field("chosen", chosen, "1 while the pad holds its value");
	}

	NumberPrompt::NumberPrompt()
		: XmlPresenter("dialog_number")
		, m_pad(*this, "number")
	{
		action("number.preset",
			   "Type a preset's value into the pad. From a row of `presets`.",
			   [this](const ActionArgs& args)
			   {
				   const auto* row = args.row<NumberPresetRow>();
				   if (row == nullptr || !m_number || static_cast<size_t>(row->index.get()) >= m_number->presets.size())
				   {
					   return;
				   }
				   // The pad tells showPresets() it changed
				   m_pad.put(m_number->presets[static_cast<size_t>(row->index.get())].value);
			   });
		m_pad.setOnChange([this] { showPresets(); });
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
		f.list("presets", m_presets, "Values offered beside the pad, e.g. materials' temperatures");
		f.field("has_presets", m_hasPresets, "1 if there are presets to show");
		f.field("heater",
				m_heater,
				"The number of the heater whose target is asked for, its temperature shown above the pad; -1 for none");
	}

	void NumberPrompt::showPresets()
	{
		const std::vector<Prompts::NumberPreset> none;
		const auto& presets = m_number ? m_number->presets : none;
		const std::string unit = m_number ? m_number->unit : "";
		const std::string typed(m_pad.text());
		m_presets.sync(presets.size(),
					   [&presets, &unit, &typed](NumberPresetRow& row, size_t i)
					   {
						   const std::string value = NumberPad::shown(presets[i].value);
						   row.title.set(presets[i].title);
						   row.value.set(unit.empty() ? value : value + " " + unit);
						   row.chosen.set(!typed.empty() && typed == value ? 1 : 0);
					   });
		m_hasPresets.set(presets.empty() ? 0 : 1);
	}

	void NumberPrompt::ask(Prompts::NumberRequest request, PromptStep& step)
	{
		m_step = &step;
		m_text.reset();
		m_pad.askNumber(request.min, request.max, request.value, false, request.unit);
		m_pad.setStep(request.step);
		m_heater.set(request.heater);
		m_number = std::move(request);
		showPresets();
	}

	void NumberPrompt::ask(Prompts::TextRequest request, PromptStep& step)
	{
		m_step = &step;
		m_number.reset();
		m_pad.askText(request.text);
		m_text = std::move(request);
		showPresets();
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

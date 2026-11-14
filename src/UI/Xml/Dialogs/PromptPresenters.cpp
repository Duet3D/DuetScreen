/*
 * PromptPresenters.cpp
 */

#include "PromptPresenters.h"
#include "PromptStep.h"
#include "i18n/i18n.h"
#include "utils/utils.h"
#include <algorithm>
#include <cstdlib>
#include <limits>

namespace UI::Xml
{
	namespace
	{
		constexpr std::string_view UNSIGNED_CHARS = "0123456789."; // What a box for a number of 0 or more takes
		constexpr std::string_view SIGNED_CHARS = "0123456789.-";  // What a box for any number takes

		/** The most decimals a number shows: a float holds no more that a user means */
		constexpr int DECIMALS = 3;

		/** `value` as the user reads it, e.g. "0.1" for 0.1f rather than "0.100000001" */
		std::string shown(float value)
		{
			return utils::formatFloat(value, DECIMALS);
		}

		/** Beyond this, a limit is none: anything can be typed that way (a slider that may go past its end) */
		constexpr float NO_LIMIT = 1e9f;

		/** The numbers that can be typed between `min` and `max`, e.g. "0 to 280" or "0 or more"; empty for any */
		std::string rangeOf(float min, float max)
		{
			const bool hasMin = min > -NO_LIMIT;
			const bool hasMax = max < NO_LIMIT;
			if (hasMin && hasMax)
			{
				return _("dialogs.range", shown(min), shown(max));
			}
			if (hasMin)
			{
				return _("dialogs.at_least", shown(min));
			}
			return hasMax ? _("dialogs.at_most", shown(max)) : std::string();
		}

		/** `text` as a number, as typed: empty is 0, and it is kept within what a float holds */
		float parse(std::string_view text)
		{
			const double value = std::strtod(std::string(text).c_str(), nullptr);
			return static_cast<float>(std::clamp(value,
												 static_cast<double>(std::numeric_limits<float>::lowest()),
												 static_cast<double>(std::numeric_limits<float>::max())));
		}
	} // namespace

	/* ---------- NumberPrompt ---------- */

	NumberPrompt::NumberPrompt()
		: XmlPresenter("dialog_number")
		, m_accepted(UNSIGNED_CHARS)
	{
		action("number.typed",
			   "What has been typed in the box changed. From the box's `value_changed`.",
			   [this](const ActionArgs& args) { setText(args.text()); });
		action("number.clear", "Empty the box", [this](const ActionArgs&) { setText(""); });
		action("number.confirm",
			   "Hand back what has been typed, if it can be, and go back. From the keys' `ready` (their OK key).",
			   [this](const ActionArgs&) { confirm(); });
	}

	void NumberPrompt::fields(FieldVisitor& f)
	{
		f.field("text", m_typed, "What has been typed");
		f.field("accepted", m_accepted, "The characters the box takes, for `bind_accepted_chars`");
		f.field("range", m_range, "The numbers that can be typed, e.g. \"0 to 280\"; empty for digits and dots");
		f.field("confirmable",
				m_confirmable,
				"1 if what has been typed can be confirmed: a number within the limits, or any text");
		f.field("signed", m_signed, "1 if the number may be negative, so the +/- key works");
	}

	void NumberPrompt::ask(Prompts::NumberRequest request, PromptStep& step)
	{
		m_step = &step;
		m_text.reset();
		m_signed.set(request.min < 0.0f ? 1 : 0);
		m_accepted.set(request.min < 0.0f ? SIGNED_CHARS : UNSIGNED_CHARS);
		m_range.set(rangeOf(request.min, request.max));
		const std::optional<float> start = request.value;
		m_number = std::move(request);
		setText(start ? shown(*start) : std::string());
	}

	void NumberPrompt::ask(Prompts::TextRequest request, PromptStep& step)
	{
		m_step = &step;
		m_number.reset();
		m_signed.set(0);
		m_accepted.set(UNSIGNED_CHARS);
		m_range.set("");
		const std::string start = request.text;
		m_text = std::move(request);
		setText(start);
	}

	void NumberPrompt::setText(std::string_view text)
	{
		m_typed.set(text);
		m_confirmable.set(confirmable() ? 1 : 0);
	}

	bool NumberPrompt::confirmable() const
	{
		if (m_number)
		{
			const float value = parse(m_typed.get());
			return value >= m_number->min && value <= m_number->max;
		}
		return m_text.has_value();
	}

	void NumberPrompt::confirm()
	{
		if (m_step == nullptr || !confirmable())
		{
			return;
		}
		const std::string typed(m_typed.get());
		if (m_number)
		{
			m_step->finish(
				[onConfirm = m_number->onConfirm, value = parse(typed)]()
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
				[onConfirm = m_text->onConfirm, typed]()
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

/*
 * PromptPresenters.h
 *
 * The presenters of the prompts code asks for (Prompts.h), one per prompt shown, each behind an XML component in
 * assets/ui/components/dialogs/:
 *
 *  - dialog_number:   a number pad (NumberPad.h), for a number within limits or for digits and dots (NumberPrompt)
 *  - dialog_choice:   a list of values, the chosen one marked (ChoicePrompt)
 *  - dialog_question: a question, with buttons to agree or not (QuestionPrompt)
 *
 * Each is given its question and its step (PromptStep.h) by Prompts, and answers through the step. Their names and
 * actions are declared in fields() and their constructors, and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "NumberPad.h"
#include "Prompts.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml
{
	class PromptStep;

	/** Behind dialog_number.xml: a number pad (NumberPad.h, its id `number`), whose OK hands the number back */
	class NumberPrompt : public XmlPresenter
	{
	  public:
		/** A prompt asking for nothing yet */
		NumberPrompt();

		/** Declare the pad's fields */
		void fields(FieldVisitor& f) override;

		/** Ask for a number, answering through `step` */
		void ask(Prompts::NumberRequest request, PromptStep& step);

		/** Ask for digits and dots, answering through `step` */
		void ask(Prompts::TextRequest request, PromptStep& step);

	  private:
		/** Hand back what is typed, if it can be */
		void confirm();

		NumberPad m_pad;								// The keys, and what they have typed
		PromptStep* m_step = nullptr;				   // Where the answer goes
		std::optional<Prompts::NumberRequest> m_number; // The number asked for, if it is one
		std::optional<Prompts::TextRequest> m_text;	   // The text asked for, if it is text
	};

	/** One value of a ChoicePrompt */
	class ChoiceRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<64> value;  // The value handed back
		StringSubject<128> title; // What the user sees
		IntSubject chosen;		  // 1 for the value chosen now
	};

	/** Behind dialog_choice.xml: the values to choose from */
	class ChoicePrompt : public XmlPresenter
	{
	  public:
		/** A prompt with nothing to choose yet */
		ChoicePrompt();

		/** Declare the list of values */
		void fields(FieldVisitor& f) override;

		/** Ask for one of `request`'s values, answering through `step` */
		void ask(Prompts::ChoiceRequest request, PromptStep& step);

	  private:
		PromptStep* m_step = nullptr;					// Where the answer goes
		std::function<void(const std::string&)> m_onChoose; // Called with the value chosen
		List<ChoiceRow> m_choices;						// `choices`
	};

	/** Behind dialog_question.xml: the question and its buttons */
	class QuestionPrompt : public XmlPresenter
	{
	  public:
		/** A prompt asking nothing yet */
		QuestionPrompt();

		/** Declare the question's fields */
		void fields(FieldVisitor& f) override;

		/** Ask `request`, answering through `step` */
		void ask(Prompts::QuestionRequest request, PromptStep& step);

	  private:
		PromptStep* m_step = nullptr;	   // Where the answer goes
		std::function<void()> m_onYes;	   // Called if the user agrees
		std::function<void()> m_onNo;	   // Called if the user doesn't

		StringSubject<512> m_text; // `text`
		StringSubject<64> m_yes;   // `yes`
		StringSubject<64> m_no;	   // `no`
	};
} // namespace UI::Xml

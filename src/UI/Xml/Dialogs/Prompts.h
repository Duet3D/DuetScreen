/*
 * Prompts.h
 *
 * Asking the user something from code: a number, digits and dots (an IP address), one of a list of choices, or a yes
 * or no question. Each opens on the dialog surface (DialogHost.h) as a step of its own: the dialog, or the next step of
 * the one open, which the card slides to. The answer is handed back to the callback once the step has gone, so the
 * callback may ask the next question; leaving without answering (the close or back button, tapping outside) runs
 * `onDismissed`, if given.
 *
 *     Prompts::askNumber({.header = "Bed active", .min = 0, .max = 120, .value = 60,
 *                         .onConfirm = [](float v) { setBed(v); }});
 *
 * Each prompt is an XML component with a presenter of its own (assets/ui/components/dialogs/dialog_*.xml,
 * PromptPresenters.h). A prompt asked for by a widget (`asker`) goes when the widget does, handing back nothing.
 */

#pragma once

#include "helix_xml.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml::Prompts
{
	/** A number to ask for */
	struct NumberRequest
	{
		std::string header{};						  // What is asked for, the prompt's title, e.g. "T0 heater 1 active"
		float min = 0.0f;						  // Smallest value accepted; below 0, the +/- key works
		float max = 0.0f;						  // Largest value accepted
		std::optional<float> value{};				  // Shown to start with; the box starts empty if not set
		std::string unit{};							  // Shown after the number and its limits, e.g. "s"; empty for none
		std::function<void(float)> onConfirm{};	  // Called with the value the user confirms
		lv_obj_t* asker = nullptr;				  // The widget asking, which the prompt goes with
	};

	/** Text typed on the number keys, such as an IP address: digits and dots */
	struct TextRequest
	{
		std::string header{};								 // What is asked for, the prompt's title
		std::string text{};								 // Shown to start with
		std::function<void(std::string_view)> onConfirm{}; // Called with the text the user confirms
		lv_obj_t* asker = nullptr;						 // The widget asking, which the prompt goes with
	};

	/** One of the values a choice can take */
	struct Choice
	{
		std::string value{}; // The value handed back
		std::string title{}; // What the user sees
	};

	/** One of a list of values to choose */
	struct ChoiceRequest
	{
		std::string title{};							  // What is chosen, e.g. a setting's name
		std::vector<Choice> choices{};				  // The values, in the order shown
		std::string chosen{};							  // The value chosen now, marked; none if it is none of them
		std::function<void(const std::string&)> onChoose; // Called with the value chosen
		std::function<void()> onDismissed = nullptr;	  // Called if the user leaves without choosing
		lv_obj_t* asker = nullptr;						  // The widget asking, which the prompt goes with
	};

	/** A question answered yes or no */
	struct QuestionRequest
	{
		std::string title{};					   // The question's title
		std::string text{};					   // The question
		std::string yes{};					   // The label of the button that agrees; "Yes" if empty
		std::string no{};						   // The label of the button that doesn't; "No" if empty
		std::function<void()> onYes{};		   // Called if the user agrees
		std::function<void()> onNo = nullptr;  // Called if the user doesn't, or leaves without answering
		lv_obj_t* asker = nullptr;			   // The widget asking, which the question goes with
	};

	/** Ask for a number on a number pad. @return false if it couldn't be shown */
	bool askNumber(NumberRequest request);

	/** Ask for digits and dots on a number pad, e.g. an IP address. @return false if it couldn't be shown */
	bool askText(TextRequest request);

	/** Ask for one of a list of values. @return false if it couldn't be shown */
	bool askChoice(ChoiceRequest request);

	/** Ask a yes or no question. @return false if it couldn't be shown */
	bool askQuestion(QuestionRequest request);
} // namespace UI::Xml::Prompts

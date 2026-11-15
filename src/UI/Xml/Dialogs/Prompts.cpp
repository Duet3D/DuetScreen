/*
 * Prompts.cpp
 */

#include "Prompts.h"
#include "DialogHost.h"
#include "PromptPresenters.h"
#include "PromptStep.h"
#include "Storage.h"
#include "utils/StorageHelper.h"
#include <memory>

namespace UI::Xml::Prompts
{
	namespace
	{
		/** The surface a prompt from `asker` opens on: the asker's screen's, else the screen shown */
		DialogHost& hostFor(lv_obj_t* asker)
		{
			return DialogHost::of(asker != nullptr ? asker : lv_screen_active());
		}

		/**
		 * Build `component` with a new `Presenter` on the surface for `asker`, give the presenter the question with
		 * `ask`, and open it
		 */
		template <typename Presenter, typename Ask>
		bool open(const char* component,
				  const std::string& title,
				  DialogStep::Size size,
				  lv_obj_t* asker,
				  std::function<void()> onDismissed,
				  Ask ask)
		{
			DialogHost& host = hostFor(asker);
			auto presenter = std::make_shared<Presenter>();
			auto step = std::make_unique<PromptStep>(host, component, presenter, title, std::move(size), asker);
			step->setOnDismissed(std::move(onDismissed));
			ask(*presenter, *step);
			host.open(std::move(step));
			return true;
		}
	} // namespace

	bool askNumber(NumberRequest request)
	{
		const std::string title = request.header;
		lv_obj_t* asker = request.asker;
		// Presets go beside the pad
		const char* width = request.presets.empty() ? "460" : "680";
		return open<NumberPrompt>("dialog_number",
								  title,
								  {.width = width, .height = "content"},
								  asker,
								  nullptr,
								  [&request](NumberPrompt& prompt, PromptStep& step)
								  { prompt.ask(std::move(request), step); });
	}

	bool askText(TextRequest request)
	{
		const std::string title = request.header;
		lv_obj_t* asker = request.asker;
		return open<NumberPrompt>("dialog_number",
								  title,
								  {.width = "460", .height = "content"},
								  asker,
								  nullptr,
								  [&request](NumberPrompt& prompt, PromptStep& step)
								  { prompt.ask(std::move(request), step); });
	}

	bool askChoice(ChoiceRequest request)
	{
		const std::string title = request.title;
		lv_obj_t* asker = request.asker;
		auto onDismissed = std::move(request.onDismissed);
		return open<ChoicePrompt>("dialog_choice",
								  title,
								  {.width = "50%", .height = "content"},
								  asker,
								  std::move(onDismissed),
								  [&request](ChoicePrompt& prompt, PromptStep& step)
								  { prompt.ask(std::move(request), step); });
	}

	bool askQuestion(QuestionRequest request)
	{
		const std::string title = request.title;
		lv_obj_t* asker = request.asker;
		auto onNo = request.onNo;
		return open<QuestionPrompt>("dialog_question",
									title,
									{.width = "60%", .height = "content"},
									asker,
									std::move(onNo),
									[&request](QuestionPrompt& prompt, PromptStep& step)
									{ prompt.ask(std::move(request), step); });
	}

	bool confirm(QuestionRequest request)
	{
		if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
		{
			if (request.onYes)
			{
				request.onYes();
			}
			return true;
		}
		return askQuestion(std::move(request));
	}
} // namespace UI::Xml::Prompts

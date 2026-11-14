/*
 * PromptStep.h
 *
 * A step code asks for (see Prompts.h): an XML component, shown with a presenter of its own that holds the question
 * and hands back the answer. The dialog surface owns it from when it opens.
 *
 * The presenter answers through finish(): the step leaves the surface at once (just this step: one opened by the
 * answer stays), then the answer is handed back. If the user leaves it without answering, its onDismissed runs. A
 * prompt asked for by a widget (its asker) goes when the widget does, and then hands back nothing.
 */

#pragma once

#include "DialogStep.h"
#include "UI/Xml/Binding/Subjects.h"
#include "helix_xml.h"
#include <functional>
#include <memory>
#include <string>

namespace UI::Xml
{
	class DialogHost;
	class XmlPresenter;
	class XmlView;

	class PromptStep : public DialogStep
	{
	  public:
		/**
		 * Build `component` with `presenter` on `host`, ready to open there
		 * @param title Its title, shown in the card's header
		 * @param size The card's size while it shows
		 * @param asker The widget asking, which the prompt goes with; nullptr for none
		 */
		PromptStep(DialogHost& host,
				   const std::string& component,
				   std::shared_ptr<XmlPresenter> presenter,
				   const std::string& title,
				   Size size,
				   lv_obj_t* asker);

		/** Stop watching the asker; the content goes with the view */
		~PromptStep() override;

		PromptStep(const PromptStep&) = delete;
		PromptStep& operator=(const PromptStep&) = delete;

		/** The component's root */
		lv_obj_t* content() override;

		/** The surface's parking place */
		lv_obj_t* home() override;

		/** The title */
		lv_subject_t* title() override;

		/** The card's size */
		Size size() const override { return m_size; }

		/** Prompts can always be left */
		bool blocking() const override { return false; }

		/** Prompts are never alerts */
		bool alert() const override { return false; }

		/** The user left without answering: run onDismissed, if set */
		void dismissed() override;

		/** Run `callback` if the user leaves without answering */
		void setOnDismissed(std::function<void()> callback) { m_onDismissed = std::move(callback); }

		/** Answer: leave the surface, then run `handBack` with the answer, unless the asker has gone */
		void finish(const std::function<void()>& handBack);

	  private:
		/** The asker is being deleted: the prompt goes, answering nothing */
		static void askerDeletedCb(lv_event_t* e);

		DialogHost& m_host;					  // The surface it opens on
		std::unique_ptr<XmlView> m_view;	  // The component, with its presenter
		StringSubject<128> m_title;			  // The title
		Size m_size;						  // The card's size
		lv_obj_t* m_asker;					  // The widget asking, or nullptr
		bool m_answered = false;			  // Answered, or left: nothing more is handed back
		std::function<void()> m_onDismissed;  // Run if the user leaves without answering
	};
} // namespace UI::Xml

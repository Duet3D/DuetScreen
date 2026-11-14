/*
 * DialogStep.h
 *
 * One step of the dialog surface (see DialogHost.h): what the card shows while the step is on top, its title and
 * size, and what its owner is told when the user leaves it without answering. A `<ds_dialog>` is one (DsDialog.h), and
 * so is each prompt code asks for: a number, a choice or a question (UI/Xml/Runtime/Prompts.h).
 */

#pragma once

#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DialogStep
	{
	  public:
		/** How big the card is while the step shows, as XML writes a size: a percentage of the room, pixels, or
		 * `content` */
		struct Size
		{
			std::string width = "70%";		// The card's width
			std::string height = "content"; // The card's height
		};

		virtual ~DialogStep() = default;

		/**
		 * The step's content. The host moves it into its card while the step is on top, and back to home() otherwise;
		 * the step must not delete it while the host has it.
		 */
		virtual lv_obj_t* content() = 0;

		/** Where the content lives while the card doesn't show it (it is hidden there) */
		virtual lv_obj_t* home() = 0;

		/** The step's title, shown in the card's header while it is on top; nullptr for none */
		virtual lv_subject_t* title() = 0;

		/** The card's size while the step shows */
		virtual Size size() const = 0;

		/** Whether the user must answer it: no close or back button, and tapping outside does nothing */
		virtual bool blocking() const = 0;

		/** Whether it is an alert: shown before the user's dialog, which is set aside until it is answered */
		virtual bool alert() const = 0;

		/**
		 * The user left the step without answering it (the close or back button, tapping outside, the back key, the
		 * page changing): tell its owner. The host has already taken it off the surface.
		 */
		virtual void dismissed() = 0;
	};
} // namespace UI::Xml

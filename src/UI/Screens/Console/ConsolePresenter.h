/*
 * ConsolePresenter.h
 *
 * Behaviour for the console page (assets/ui/screens/console.xml): send G-code, with a list of the G-codes the Duet
 * knows, narrowed to those that start with what has been typed. What was sent and what came back is the global
 * `console.lines` (src/UI/Xml/Data/ConsoleSource.h).
 *
 * Fields
 *   input              what is typed (kept equal to the text box, so picking a G-code can replace it)
 *   commands_shown     1 when the G-code list is showing, 0 when it is folded away (remembered)
 *   gcodes             list, the G-codes starting with the first word typed
 *     gcode            e.g. "G28"
 *     description      e.g. "Home"
 *
 * Actions
 *   console.input            (value_changed on the text box) what is typed changed
 *   console.send             send what is typed
 *   console.clear            empty the text box
 *   console.pick             (in a gcodes row) put that G-code in the text box
 *   console.toggle_commands  show or fold away the G-code list
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string_view>

namespace UI
{
	/** One G-code in the console's list */
	class GcodeRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<16> gcode;		 // E.g. "G28"
		Xml::StringSubject<128> description; // What it does
	};

	class ConsolePresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with every G-code listed */
		ConsolePresenter();

		/** Declare the console's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Show the G-code list as it was left */
		void onInit() override;

	  private:
		/** List the G-codes starting with the first word of `typed`, ignoring case */
		void filter(std::string_view typed);

		Xml::StringSubject<256> m_input;  // What is typed
		Xml::IntSubject m_commandsShown;  // 1 when the G-code list is showing
		Xml::List<GcodeRow> m_gcodes;	  // G-codes matching what is typed
	};
} // namespace UI

/*
 * ConsolePresenter.h
 *
 * Behaviour for the console page (assets/ui/components/console/console.xml): send G-code, with a list of the G-codes
 * the Duet knows, narrowed to those that start with what has been typed. What was sent and what came back is the global
 * `console.lines` (src/UI/Xml/Data/ConsoleSource.h).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md).
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

		/** Run `command` in the shell and show its output in the console */
		static void runInShell(const std::string& command);

		Xml::StringSubject<256> m_input;  // What is typed
		Xml::IntSubject m_commandsShown;  // 1 when the G-code list is showing
		Xml::IntSubject m_shellAvailable; // 1 if this build can run shell commands
		Xml::IntSubject m_shell;		  // 1 while commands go to the shell
		Xml::List<GcodeRow> m_gcodes;	  // G-codes matching what is typed
	};
} // namespace UI

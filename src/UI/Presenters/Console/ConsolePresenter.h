/*
 * ConsolePresenter.h
 *
 * Behaviour for the console page (assets/ui/components/console/console.xml): send G-code, put a recent command or a
 * G-code from the reference back in the text box, and search the reference: the G-codes the Duet knows, narrowed to
 * those whose code starts with what is searched for or whose description contains it. What was sent and what came
 * back, and the recent commands, are the global `source.console.*` (src/UI/Xml/Data/ConsoleSource.h).
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
	/** One G-code in the console's reference */
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

	  private:
		/** Search the reference for `search`: list the G-codes it matches (see the file comment), all if it is empty */
		void filter(std::string_view search);

		/** Run `command` in the shell and show its output in the console */
		static void runInShell(const std::string& command);

		Xml::StringSubject<256> m_input;  // What is typed
		Xml::StringSubject<64> m_search;  // What the reference is searched for
		Xml::IntSubject m_referenceOpen;  // 1 while the reference dialog is open
		Xml::IntSubject m_shellAvailable; // 1 if this build can run shell commands
		Xml::IntSubject m_shell;		  // 1 while commands go to the shell
		Xml::List<GcodeRow> m_gcodes;	  // G-codes matching the search
	};
} // namespace UI

/*
 * GcodeReferencePresenter.h
 *
 * Behaviour for the G-code reference: the G-codes the Duet knows, narrowed to those whose code starts with what is
 * searched for or whose description contains it. The console page opens it as a dialog (console.xml); placed as the
 * `console_command_list` widget it is a list. Picking a G-code types it into the console's input, which every view
 * shares (`source.console.input`, src/UI/Xml/Data/ConsoleSource.h). Each view has its own search.
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
	/** One G-code in the reference */
	class GcodeRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<16> gcode;		 // E.g. "G28"
		Xml::StringSubject<128> description; // What it does
	};

	class GcodeReferencePresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter, with every G-code listed */
		GcodeReferencePresenter();

		/** Declare the reference's fields */
		void fields(Xml::FieldVisitor& f) override;

	  private:
		/** Search the reference for `search`: list the G-codes it matches (see the file comment), all if it is empty */
		void filter(std::string_view search);

		Xml::StringSubject<64> m_search; // What the reference is searched for
		Xml::IntSubject m_open;			 // 1 while the reference dialog is open
		Xml::List<GcodeRow> m_gcodes;	 // G-codes matching the search
	};
} // namespace UI

/*
 * ConsolePresenter.cpp
 */

#include "ConsolePresenter.h"
#include "Debug.h"
#include "Gcodes.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace UI
{
	void GcodeRow::fields(Xml::FieldVisitor& f)
	{
		f.field("gcode", gcode, "E.g. G28");
		f.field("description", description, "What it does");
	}

	ConsolePresenter::ConsolePresenter()
		: XmlPresenter("console")
	{
		action("console.input",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_input.set(args.text());
				   filter(args.text());
			   });
		action("console.send",
			   [this](const Xml::ActionArgs&)
			   {
				   const std::string command(m_input.get());
				   if (command.empty())
				   {
					   return;
				   }
				   Model::get().post<EventType::ConsoleCommand>(command); // Shown in the console's output
				   Xml::Services::sendGcode(command);
			   });
		action("console.clear", [this](const Xml::ActionArgs&) { m_input.set(""); });
		action("console.pick",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<GcodeRow>())
				   {
					   m_input.set(row->gcode.get());
				   }
			   });
		action("console.toggle_commands",
			   [this](const Xml::ActionArgs&)
			   {
				   const bool show = m_commandsShown.get() == 0;
				   m_commandsShown.set(show ? 1 : 0);
				   StorageHelper::setData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED, !show);
			   });
		filter("");
	}

	void ConsolePresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("input", m_input, "What is typed");
		f.field("commands_shown", m_commandsShown, "1 when the G-code list is showing");
		f.list("gcodes", m_gcodes, "The G-codes starting with the first word typed");
	}

	void ConsolePresenter::onInit()
	{
		m_commandsShown.set(StorageHelper::getData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED) ? 0 : 1);
	}

	void ConsolePresenter::filter(std::string_view typed)
	{
		ZoneScoped;
		std::string word(typed.substr(0, typed.find(' ')));
		std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::toupper(c); });

		std::vector<const gcode*> matches;
		for (size_t i = 0; i < Gcodes::getGcodeCount(); i++)
		{
			const gcode* g = Gcodes::getGcode(i);
			if (g->gcode.starts_with(word))
			{
				matches.push_back(g);
			}
		}
		m_gcodes.sync(matches.size(),
					  [&](GcodeRow& row, size_t i)
					  {
						  row.gcode.set(matches[i]->gcode);
						  row.description.set(matches[i]->helpText);
					  });
	}
} // namespace UI

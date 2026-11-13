/*
 * ConsolePresenter.cpp
 */

#include "ConsolePresenter.h"
#include "Debug.h"
#include "Gcodes.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Runtime/Services.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
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
				   if (m_shell.get() != 0)
				   {
					   runInShell(command);
					   return;
				   }
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
		action("console.shell",
			   [this](const Xml::ActionArgs& args)
			   { m_shell.set(m_shellAvailable.get() != 0 && args.value().value_or(0) != 0 ? 1 : 0); });
		action("console.toggle_commands",
			   [this](const Xml::ActionArgs&)
			   {
				   const bool show = m_commandsShown.get() == 0;
				   m_commandsShown.set(show ? 1 : 0);
				   StorageHelper::setData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED, !show);
			   });
#if ENABLE_CONSOLE_SHELL
		m_shellAvailable.set(1);
#endif
		filter("");
	}

	void ConsolePresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("input", m_input, "What is typed");
		f.field("commands_shown", m_commandsShown, "1 when the G-code list is showing");
		f.field("shell_available", m_shellAvailable, "1 if this build can run shell commands");
		f.field("shell", m_shell, "1 while commands go to the shell");
		f.list("gcodes", m_gcodes, "The G-codes starting with the first word typed");
	}

	void ConsolePresenter::onInit()
	{
		m_commandsShown.set(StorageHelper::getData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED) ? 0 : 1);
	}

	void ConsolePresenter::runInShell([[maybe_unused]] const std::string& command)
	{
#if ENABLE_CONSOLE_SHELL
		std::string output;
		if (FILE* pipe = ::popen(command.c_str(), "r"))
		{
			char buffer[256];
			while (fgets(buffer, sizeof(buffer), pipe))
			{
				output.append(buffer);
			}
			::pclose(pipe);
		}
		Model::get().post<EventType::Response>(ResponseType::INFO, output);
#endif
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

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
		f.field("gcode", gcode, "The G-code, e.g. \"G28\"");
		f.field("description", description, "What it does, e.g. \"Home\"");
	}

	ConsolePresenter::ConsolePresenter()
		: XmlPresenter("console")
	{
		action("console.input",
			   "What is typed changed to the text box's text (`value_changed` on it): narrow the G-code list to match",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_input.set(args.text());
				   filter(args.text());
			   });
		action("console.send",
			   "Send what is typed: to the shell while `shell` is 1, else to the printer as G-code",
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
		action("console.clear", "Empty the text box", [this](const Xml::ActionArgs&) { m_input.set(""); });
		action("console.pick",
			   "Put the G-code in the text box. From a row of `gcodes`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<GcodeRow>())
				   {
					   m_input.set(row->gcode.get());
				   }
			   });
		action("console.shell",
			   "Send to the shell while the edited value is non-zero (`value_changed` on a checkbox), if "
			   "`shell_available`",
			   [this](const Xml::ActionArgs& args)
			   { m_shell.set(m_shellAvailable.get() != 0 && args.value().value_or(0) != 0 ? 1 : 0); });
		action("console.toggle_commands",
			   "Show or fold away the G-code list, and remember which",
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
		f.field("input", m_input, "What is typed, kept equal to the text box so picking a G-code can replace it");
		f.field("commands_shown",
				m_commandsShown,
				"1 when the G-code list is showing, 0 when it is folded away; remembered");
		f.field("shell_available",
				m_shellAvailable,
				"1 if this build can run shell commands (ENABLE_CONSOLE_SHELL, e.g. developer builds)");
		f.field("shell", m_shell, "1 while what is sent goes to the shell instead of the Duet");
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

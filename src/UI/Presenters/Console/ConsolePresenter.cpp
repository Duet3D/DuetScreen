/*
 * ConsolePresenter.cpp
 */

#include "ConsolePresenter.h"
#include "Debug.h"
#include "Gcodes.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/ConsoleSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <vector>

namespace UI
{
	namespace
	{
		/** `text` in upper case */
		std::string upper(std::string_view text)
		{
			std::string result(text);
			std::transform(
				result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
			return result;
		}
	} // namespace

	void GcodeRow::fields(Xml::FieldVisitor& f)
	{
		f.field("gcode", gcode, "The G-code, e.g. \"G28\"");
		f.field("description", description, "What it does, e.g. \"Home\"");
	}

	ConsolePresenter::ConsolePresenter()
		: XmlPresenter("console")
	{
		action("console.input",
			   "What is typed changed to the text box's text (`value_changed` on it)",
			   [this](const Xml::ActionArgs& args) { m_input.set(args.text()); });
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
		action("console.recall",
			   "Put a recent command in the text box, to send again. From a row of `source.console.recent`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<Xml::ConsoleRecentRow>())
				   {
					   m_input.set(row->command.get());
				   }
			   });
		action("console.open_reference",
			   "Open the G-code reference, searched for the first word typed",
			   [this](const Xml::ActionArgs&)
			   {
				   const std::string_view typed = m_input.get();
				   const std::string word(typed.substr(0, typed.find(' ')));
				   m_search.set(word);
				   filter(word);
				   m_referenceOpen.set(1);
			   });
		action("console.close_reference",
			   "Close the G-code reference (`dismissed` on its dialog)",
			   [this](const Xml::ActionArgs&) { m_referenceOpen.set(0); });
		action("console.search",
			   "Search the reference for the search box's text (`value_changed` on it)",
			   [this](const Xml::ActionArgs& args)
			   {
				   m_search.set(args.text());
				   filter(args.text());
			   });
		action("console.pick",
			   "Put the G-code in the text box and close the reference. From a row of `gcodes`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<GcodeRow>())
				   {
					   m_input.set(row->gcode.get());
				   }
				   m_referenceOpen.set(0);
			   });
		action("console.shell",
			   "Send to the shell while the edited value is non-zero (`value_changed` on a checkbox), if "
			   "`shell_available`",
			   [this](const Xml::ActionArgs& args)
			   { m_shell.set(m_shellAvailable.get() != 0 && args.value().value_or(0) != 0 ? 1 : 0); });
#if ENABLE_CONSOLE_SHELL
		m_shellAvailable.set(1);
#endif
		filter("");
	}

	void ConsolePresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("input", m_input, "What is typed, kept equal to the text box so picking a G-code can replace it");
		f.field("search", m_search, "What the G-code reference is searched for");
		f.field("reference_open", m_referenceOpen, "1 while the G-code reference dialog is open");
		f.field("shell_available",
				m_shellAvailable,
				"1 if this build can run shell commands (ENABLE_CONSOLE_SHELL, e.g. developer builds)");
		f.field("shell", m_shell, "1 while what is sent goes to the shell instead of the Duet");
		f.list("gcodes",
			   m_gcodes,
			   "The G-codes the search matches: their code starts with it, or their description contains it, ignoring "
			   "case; every G-code while the search is empty");
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

	void ConsolePresenter::filter(std::string_view search)
	{
		ZoneScoped;
		const std::string wanted = upper(search);

		struct Match
		{
			const gcode* code;		 // The G-code
			std::string description; // What it does, translated
		};
		std::vector<Match> matches;
		for (size_t i = 0; i < Gcodes::getGcodeCount(); i++)
		{
			const gcode* g = Gcodes::getGcode(i);
			std::string description(_(g->helpKey));
			if (g->gcode.starts_with(wanted) || upper(description).find(wanted) != std::string::npos)
			{
				matches.push_back({g, std::move(description)});
			}
		}
		m_gcodes.sync(matches.size(),
					  [&](GcodeRow& row, size_t i)
					  {
						  row.gcode.set(matches[i].code->gcode);
						  row.description.set(matches[i].description);
					  });
	}
} // namespace UI

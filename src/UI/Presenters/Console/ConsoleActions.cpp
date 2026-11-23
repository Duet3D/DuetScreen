/*
 * ConsoleActions.cpp
 */

#include "ConsoleActions.h"
#include "Comm/Responses.h"
#include "Debug.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/ConsoleSource.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include <cstdio>
#include <string>

namespace UI::ConsoleActions
{
	namespace
	{
		/** The console's source; nullptr, with a warning, while the data sources aren't running */
		Xml::ConsoleSource* console()
		{
			auto* source = Xml::DataSources::find<Xml::ConsoleSource>();
			if (source == nullptr)
			{
				LOG_WARN("The console's source isn't running");
			}
			return source;
		}

		/** Run `command` in the shell and show its output in the console */
		void runInShell([[maybe_unused]] const std::string& command)
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

		/** `actions.console.send` */
		void send(const Xml::ActionArgs&)
		{
			auto* source = console();
			if (source == nullptr || source->input().empty())
			{
				return;
			}
			const std::string command(source->input());
			Model::get().post<EventType::ConsoleCommand>(command); // Shown in the console's output
			if (source->shell())
			{
				runInShell(command);
				return;
			}
			Xml::Services::sendGcode(command);
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("console.input",
								"What is typed changed to the text box's text (`value_changed` on it)",
								[](const Xml::ActionArgs& args)
								{
									if (auto* source = console())
									{
										source->setInput(args.text());
									}
								});
		Xml::Actions::addGlobal(
			"console.send", "Send what is typed: to the shell while `shell` is 1, else to the printer as G-code", send);
		Xml::Actions::addGlobal("console.clear",
								"Empty the text box",
								[](const Xml::ActionArgs&)
								{
									if (auto* source = console())
									{
										source->setInput("");
									}
								});
		Xml::Actions::addGlobal("console.recall",
								"Put a recent command in the text box, to send again. From a row of "
								"`source.console.recent`.",
								[](const Xml::ActionArgs& args)
								{
									const auto* row = args.row<Xml::ConsoleRecentRow>();
									auto* source = console();
									if (row != nullptr && source != nullptr)
									{
										source->setInput(row->command.get());
									}
								});
		Xml::Actions::addGlobal("console.shell",
								"Send to the shell while the edited value is non-zero (`value_changed` on a "
								"checkbox), if `shell_available`",
								[](const Xml::ActionArgs& args)
								{
									if (auto* source = console())
									{
										source->setShell(args.value().value_or(0) != 0);
									}
								});
	}
} // namespace UI::ConsoleActions

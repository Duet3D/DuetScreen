/*
 * SettingsActions.cpp
 */

#include "SettingsActions.h"
#include "Debug.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/View/Actions.h"
#include <charconv>
#include <string>

namespace UI::SettingsActions
{
	namespace
	{
		/** The settings' source; nullptr, with a warning, while the data sources aren't running */
		Xml::SettingsSource* settings()
		{
			auto* source = Xml::DataSources::find<Xml::SettingsSource>();
			if (source == nullptr)
			{
				LOG_WARN("The settings' source isn't running");
			}
			return source;
		}

		/** `actions.settings.set` */
		void set(const Xml::ActionArgs& args)
		{
			auto* source = settings();
			if (source == nullptr)
			{
				return;
			}
			// `name=value`: that value, e.g. from one of a row of buttons
			if (const size_t equals = args.arg.find('='); equals != std::string::npos)
			{
				const std::string name = args.arg.substr(0, equals);
				const std::string text = args.arg.substr(equals + 1);
				float value = 0;
				const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
				if (error != std::errc() || end != text.data() + text.size() || !source->set(name, value))
				{
					LOG_WARN("actions.settings.set: can't set '{:s}' to '{:s}'", name, text);
				}
				return;
			}
			// `name`: the edited value, or the text typed
			if (source->textSetting(args.arg) != nullptr)
			{
				source->set(args.arg, std::string_view(args.text()));
				return;
			}
			const auto edited = args.edited();
			if (!edited || !source->set(args.arg, *edited))
			{
				LOG_WARN("actions.settings.set: no edited value for a setting called '{:s}'", args.arg);
			}
		}

		/** `actions.settings.ask` */
		void ask(const Xml::ActionArgs& args)
		{
			auto* source = settings();
			const Xml::TextSetting* setting = source != nullptr ? source->textSetting(args.arg) : nullptr;
			if (setting == nullptr || !setting->askHeader)
			{
				LOG_WARN("actions.settings.ask: no text setting called '{:s}' to ask for", args.arg);
				return;
			}
			const std::string name = args.arg;
			Xml::Prompts::askText({.header = setting->askHeader(),
								   .text = setting->read(),
								   .onConfirm = [name](std::string_view text)
								   {
									   if (auto* source = settings())
									   {
										   source->set(name, text);
									   }
								   }});
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("settings.set",
								"Change the setting `arg` names (source.settings.<name>) to the edited value: a "
								"switch's 0 or 1, a choice's index, a slider's value, or a text area's text. "
								"`arg=\"<name>=<value>\"` sets it to that value instead, e.g. from a row of buttons.",
								set);
		Xml::Actions::addGlobal(
			"settings.ask", "Ask for the text setting `arg` names on the number keys, e.g. the Duet's IP address", ask);
	}
} // namespace UI::SettingsActions

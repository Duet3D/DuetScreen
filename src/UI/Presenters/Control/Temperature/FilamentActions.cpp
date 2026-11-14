/*
 * FilamentActions.cpp
 */

#include "FilamentActions.h"
#include "Debug.h"
#include "ObjectModel/Files.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/View/Actions.h"
#include "i18n/i18n.h"
#include <fmt/format.h>
#include <optional>
#include <string>
#include <vector>

namespace UI::FilamentActions
{
	namespace
	{
		/** The tool an action is about: its `arg`, if it is a tool that takes filament */
		std::optional<size_t> toolOf(const Xml::ActionArgs& args)
		{
			const auto number = args.argInt();
			if (!number || *number < 0)
			{
				LOG_WARN("actions.filament.* need the tool's number as their arg, e.g. arg=\"{{row.index}}\"");
				return std::nullopt;
			}
			MODEL_LOCK();
			if (!Xml::PrinterModel::takesFilament(static_cast<size_t>(*number)))
			{
				return std::nullopt;
			}
			return static_cast<size_t>(*number);
		}

		/** Load `filament` into `tool`: select the tool, unload what is loaded, load it and run its config */
		void load(size_t tool, const std::string& filament)
		{
			std::string loaded;
			{
				MODEL_LOCK();
				loaded = Xml::PrinterModel::filament(tool);
			}
			if (loaded == filament)
			{
				return;
			}
			Xml::Services::sendGcode(
				fmt::format("T{:d}\n{:s}M701 S\"{:s}\"\nM703\n", tool, loaded.empty() ? "" : "M702\n", filament));
		}

		/** `actions.filament.choose` */
		void choose(const Xml::ActionArgs& args)
		{
			const auto tool = toolOf(args);
			if (!tool)
			{
				return;
			}
			std::vector<std::string> filaments;
			std::string loaded;
			{
				MODEL_LOCK();
				filaments = OM::FileSystem::GetFilamentList();
				loaded = Xml::PrinterModel::filament(*tool);
			}
			OM::FileSystem::RequestFilaments(); // Fresh for next time
			if (filaments.empty())
			{
				Model::get().post<EventType::Response>(ResponseType::WARNING, _("filament.none"));
				return;
			}
			Xml::Prompts::ChoiceRequest request{.title = _("filament.select"),
												.chosen = loaded,
												.onChoose = [number = *tool](const std::string& filament)
												{ load(number, filament); },
												.asker = args.target};
			for (const auto& filament : filaments)
			{
				request.choices.push_back({.value = filament, .title = filament});
			}
			Xml::Prompts::askChoice(std::move(request));
		}

		/** `actions.filament.unload` */
		void unload(const Xml::ActionArgs& args)
		{
			const auto tool = toolOf(args);
			if (!tool)
			{
				return;
			}
			{
				MODEL_LOCK();
				if (Xml::PrinterModel::filament(*tool).empty())
				{
					return;
				}
			}
			Xml::Services::sendGcode(fmt::format("T{:d}\nM702\n", *tool));
		}
	} // namespace

	void registerAll()
	{
		Xml::Actions::addGlobal("filament.choose",
								"Ask which of the printer's filaments to load into the tool numbered `arg`, and load "
								"it, unloading any loaded first (M701, M702, M703)",
								choose);
		Xml::Actions::addGlobal(
			"filament.unload", "Unload the filament of the tool numbered `arg` (M702), if it has any loaded", unload);
	}
} // namespace UI::FilamentActions

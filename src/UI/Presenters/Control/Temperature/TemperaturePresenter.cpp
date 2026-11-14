/*
 * TemperaturePresenter.cpp
 */

#include "TemperaturePresenter.h"
#include "Debug.h"
#include "ObjectModel/Files.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Runtime/Services.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI
{
	void TemperatureToolRow::fields(Xml::FieldVisitor& f)
	{
		f.field("number", number, "Tool number, as used by T; the tool's own fields are om.tools[{row.number}]");
		f.field("filament_shown", filamentShown, "1 if the tool takes filament");
		f.field("filament_text", filamentText, "The filament picked or loaded, or \"No Filament Loaded\"");
		f.field("filament_loaded", filamentLoaded, "1 if filament is loaded, so it can be unloaded");
		f.field("change_shown", changeShown, "1 if a filament other than the loaded one has been picked");
		f.field("change_text", changeText, "\"Load\", or \"Change\" when filament is loaded");
	}

	void TemperatureToolRow::update(size_t toolNumber)
	{
		number.set(static_cast<int32_t>(toolNumber));
		filamentShown.set(Xml::PrinterModel::takesFilament(toolNumber) ? 1 : 0);

		// A new filament loaded replaces whatever was picked
		std::string filament(Xml::PrinterModel::filament(toolNumber));
		if (filament != loaded)
		{
			loaded = std::move(filament);
			picked = loaded;
		}
		showFilament();
	}

	void TemperatureToolRow::pick(std::string filament)
	{
		picked = std::move(filament);
		showFilament();
	}

	void TemperatureToolRow::showFilament()
	{
		filamentText.set(picked.empty() ? _("filament.not_loaded") : picked);
		filamentLoaded.set(loaded.empty() ? 0 : 1);
		changeShown.set(picked != loaded ? 1 : 0);
		changeText.set(loaded.empty() ? _("filament.load") : _("filament.change"));
	}

	TemperaturePresenter::TemperaturePresenter()
		: XmlPresenter("temperature")
	{
		action("temperature.pick_filament",
			   "Pick the filament at the confirmed index of `filaments` (`value_confirmed`) to load next. From a row "
			   "of `presenter.tools`.",
			   [this](const Xml::ActionArgs& args)
			   {
				   auto* row = args.row<TemperatureToolRow>();
				   const auto index = args.value();
				   if (row != nullptr && index && *index >= 0 && static_cast<size_t>(*index) < m_filamentNames.size())
				   {
					   row->pick(m_filamentNames[static_cast<size_t>(*index)]);
				   }
			   });
		action("temperature.change_filament",
			   "Load the filament picked, unloading any loaded first. From a row of `presenter.tools`.",
			   [](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<TemperatureToolRow>())
				   {
					   changeFilament(static_cast<size_t>(row->number.get()), row->picked);
				   }
			   });
		action("temperature.unload_filament",
			   "Unload the tool's filament. From a row of `presenter.tools`.",
			   [](const Xml::ActionArgs& args)
			   {
				   if (const auto* row = args.row<TemperatureToolRow>())
				   {
					   unloadFilament(static_cast<size_t>(row->number.get()));
				   }
			   });
	}

	void TemperaturePresenter::fields(Xml::FieldVisitor& f)
	{
		f.list("tools", m_tools, "The tools with heaters");
		f.field("filaments", m_filaments, "The filaments the printer knows, one per line");
		f.field("selected_tool", m_selectedTool, "E.g. \"Selected Tool: Tool 0\"");
	}

	void TemperaturePresenter::onInit()
	{
		m_watch = Xml::OmWatch({"tools", "heat", "state.currentTool", "sensors.analog", "move.extruders"},
							   [this] { showTools(); });
		registerEventListener<EventType::Filaments>([this](const std::vector<std::string>& filaments)
													{ showFilaments(filaments); });
	}

	void TemperaturePresenter::onActivate()
	{
		showTools();
		{
			MODEL_LOCK();
			showFilaments(OM::FileSystem::GetFilamentList());
		}
		OM::FileSystem::RequestFilaments();
	}

	void TemperaturePresenter::onDisconnect()
	{
		m_tools.clear();
		m_selectedTool.set(_("temperature.selected_tool_label", ""));
	}

	void TemperaturePresenter::showTools()
	{
		ZoneScoped;
		MODEL_LOCK();
		std::vector<size_t> heated;
		for (const size_t number : Xml::PrinterModel::tools())
		{
			if (Xml::PrinterModel::tool(number)["heaters"].size() > 0)
			{
				heated.push_back(number);
			}
		}
		m_tools.sync(heated.size(), [&heated](TemperatureToolRow& row, size_t i) { row.update(heated[i]); });

		const auto current = Xml::PrinterModel::currentTool();
		m_selectedTool.set(
			_("temperature.selected_tool_label", current ? Xml::PrinterModel::toolName(*current) : std::string()));
	}

	void TemperaturePresenter::showFilaments(const std::vector<std::string>& filaments)
	{
		m_filamentNames = filaments;
		m_filaments.set(fmt::format("{}", fmt::join(filaments, "\n")));
	}

	void TemperaturePresenter::changeFilament(size_t tool, const std::string& filament)
	{
		std::string loaded;
		{
			MODEL_LOCK();
			if (!Xml::PrinterModel::takesFilament(tool))
			{
				LOG_WARN("Tool {:d} has no filament extruder", tool);
				return;
			}
			loaded = Xml::PrinterModel::filament(tool);
		}
		if (loaded == filament)
		{
			return;
		}
		if (filament.empty())
		{
			unloadFilament(tool);
			return;
		}
		// Select the tool, unload what is loaded, load the new filament and run its config
		Xml::Services::sendGcode(
			fmt::format("T{:d}\n{:s}M701 S\"{:s}\"\nM703\n", tool, loaded.empty() ? "" : "M702\n", filament));
	}

	void TemperaturePresenter::unloadFilament(size_t tool)
	{
		{
			MODEL_LOCK();
			if (!Xml::PrinterModel::takesFilament(tool) || Xml::PrinterModel::filament(tool).empty())
			{
				return;
			}
		}
		Xml::Services::sendGcode(fmt::format("T{:d}\nM702\n", tool));
	}
} // namespace UI

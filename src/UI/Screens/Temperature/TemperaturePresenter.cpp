/*
 * TemperaturePresenter.cpp
 */

#include "TemperaturePresenter.h"
#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/Tool.h"
#include "UI/Core/Model.h"
#include "UI/Core/Navigation.h"
#include "UI/Widgets/Control/ExtrusionFactor/ExtrusionFactor.h"
#include "i18n/i18n.h"
#include <fmt/format.h>

namespace UI
{
	void ToolExtruderRow::fields(Xml::FieldVisitor& f)
	{
		f.field("factor", factor, "Extrusion factor, e.g. \"100%\"");
	}

	void TemperatureToolRow::fields(Xml::FieldVisitor& f)
	{
		ToolRow::fields(f);
		f.field("state", state, "Status key, e.g. active, standby, off");
		f.field("status", status, "Status, translated for display");
		f.list("extruders", extruders, "The tool's extruders, each with `factor`");
		f.field("filament_shown", filamentShown, "1 if the tool takes filament");
		f.field("filament_text", filamentText, "The filament picked or loaded, or \"No Filament Loaded\"");
		f.field("filament_loaded", filamentLoaded, "1 if filament is loaded, so it can be unloaded");
		f.field("change_shown", changeShown, "1 if a filament other than the loaded one has been picked");
		f.field("change_text", changeText, "\"Load\", or \"Change\" when filament is loaded");
	}

	void TemperatureToolRow::update(OM::Tool& tool)
	{
		ToolRow::update(tool);
		state.set(tool.GetStatusStr());
		status.set(_(fmt::format("temperature.status.{:s}", tool.GetStatusStr())));
		extruders.sync(tool.GetExtruderCount(),
					   [&tool](ToolExtruderRow& row, size_t i)
					   {
						   if (auto extruder = tool.GetExtruder(i))
						   {
							   row.factor.set(fmt::format("{:d}%", static_cast<int>(100 * extruder->factor)));
						   }
					   });
		filamentShown.set(tool.filamentExtruder >= 0 ? 1 : 0);

		// A new filament loaded replaces whatever was picked
		std::string filament(tool.GetFilament().c_str());
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
		action("temperature.extrusion_factor",
			   "Open the dialog to change the tool's extrusion factors. From a row of `tools`.",
			   [](const Xml::ActionArgs& args)
			   {
				   auto tool = toolOf(args);
				   auto* dialog = findView<ModalExtrusionFactor>("extrusion_factor");
				   if (tool == nullptr || dialog == nullptr)
				   {
					   return;
				   }
				   dialog->getExtrusionFactor().getPresenter()->setTool(tool);
				   openModal(dialog);
			   });
		action("temperature.pick_filament",
			   "Pick the dropdown's chosen filament (`value_changed`) to load next. From a row of `tools`.",
			   [](const Xml::ActionArgs& args)
			   {
				   if (auto* row = args.row<TemperatureToolRow>())
				   {
					   row->pick(args.text());
				   }
			   });
		action("temperature.change_filament",
			   "Load the filament picked, unloading any loaded first. From a row of `tools`.",
			   [](const Xml::ActionArgs& args)
			   {
				   const auto* row = args.row<TemperatureToolRow>();
				   if (auto tool = toolOf(args); tool && row)
				   {
					   tool->ChangeFilament(row->picked);
				   }
			   });
		action("temperature.unload_filament",
			   "Unload the tool's filament. From a row of `tools`.",
			   [](const Xml::ActionArgs& args)
			   {
				   if (auto tool = toolOf(args))
				   {
					   tool->UnloadFilament();
				   }
			   });
	}

	void TemperaturePresenter::fields(Xml::FieldVisitor& f)
	{
		f.list("tools", m_tools, "The tools with heaters; each row also has the fields of a global `tools` row");
		f.field("filaments", m_filaments, "The filaments the printer knows, one per line");
		f.field("selected_tool", m_selectedTool, "E.g. \"Selected Tool: Tool 0\"");
	}

	void TemperaturePresenter::onInit()
	{
		registerEventListener<EventType::ToolData>([this]() { showTools(); });
		registerEventListener<EventType::ToolHeaterData>([this](size_t) { showTools(); });
		registerEventListener<EventType::HeaterData>([this]() { showTools(); });
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
		std::vector<OM::ToolPtr> heated;
		heated.reserve(OM::GetToolCount());
		OM::IterateToolsWhile(
			[&heated](OM::ToolPtr tool, size_t)
			{
				if (tool->GetHeaterCount() > 0)
				{
					heated.push_back(tool);
				}
				return true;
			});
		m_tools.sync(heated.size(), [&heated](TemperatureToolRow& row, size_t i) { row.update(*heated[i]); });

		const auto current = OM::GetCurrentTool();
		m_selectedTool.set(_("temperature.selected_tool_label", current ? current->GetName() : std::string()));
	}

	void TemperaturePresenter::showFilaments(const std::vector<std::string>& filaments)
	{
		m_filaments.set(fmt::format("{}", fmt::join(filaments, "\n")));
	}

	OM::ToolPtr TemperaturePresenter::toolOf(const Xml::ActionArgs& args)
	{
		const auto* row = args.row<Xml::ToolRow>();
		if (row == nullptr)
		{
			LOG_WARN("{:s} must come from a tools row", args.name);
			return nullptr;
		}
		return OM::GetTool(static_cast<size_t>(row->number.get()));
	}
} // namespace UI

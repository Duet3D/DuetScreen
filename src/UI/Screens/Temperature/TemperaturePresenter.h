/*
 * TemperaturePresenter.h
 *
 * Behaviour for the Temperature tab (assets/ui/screens/temperature.xml): every tool with a heater, with its state,
 * extrusion factors, filament and heaters; the beds and chambers (the global `beds` and `chambers` lists,
 * src/UI/Xml/Data/HeatSource.h); and the extruder panel (extruder.xml).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md). Tool selection and heater states and temperatures use the global actions in
 * HeaterActions.h.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Data/ToolSource.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>
#include <vector>

namespace OM
{
	struct Tool;
}

namespace UI
{
	/** One extruder of a tool */
	class ToolExtruderRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<8> factor; // Extrusion factor, e.g. "100%"
	};

	/** One tool with heaters, as the Temperature tab shows it */
	class TemperatureToolRow : public Xml::ToolRow
	{
	  public:
		/** Declare the row's fields: a tools row's, and the tab's own */
		void fields(Xml::FieldVisitor& f) override;

		/** Copy `tool` into the subjects */
		void update(OM::Tool& tool);

		/** Pick `filament`, to load with change_filament */
		void pick(std::string filament);

		Xml::StringSubject<16> state;			  // Status key
		Xml::StringSubject<32> status;			  // Translated status
		Xml::List<ToolExtruderRow> extruders;	  // The tool's extruders
		Xml::IntSubject filamentShown;			  // 1 if the tool takes filament
		Xml::StringSubject<64> filamentText;	  // Picked or loaded filament, or "No Filament Loaded"
		Xml::IntSubject filamentLoaded;			  // 1 if filament is loaded
		Xml::IntSubject changeShown;			  // 1 if another filament has been picked
		Xml::StringSubject<32> changeText;		  // "Load" or "Change"
		std::string picked;						  // Filament picked; not shown
		std::string loaded;						  // Filament loaded; not shown

	  private:
		/** Show the filament picked and loaded */
		void showFilament();
	};

	class TemperaturePresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		TemperaturePresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for tools, heaters and filaments */
		void onInit() override;

		/** Show the tools and ask the printer for its filaments */
		void onActivate() override;

		/** Forget the tools */
		void onDisconnect() override;

	  private:
		/** Show every tool with heaters, and the selected tool */
		void showTools();

		/** Show the filaments the printer knows */
		void showFilaments(const std::vector<std::string>& filaments);

		/** The tool an action's row is about, or nullptr */
		static std::shared_ptr<OM::Tool> toolOf(const Xml::ActionArgs& args);

		Xml::List<TemperatureToolRow> m_tools;	   // Tools with heaters
		Xml::StringSubject<2048> m_filaments;	   // Known filaments, one per line
		Xml::StringSubject<64> m_selectedTool;	   // "Selected Tool: …"
	};
} // namespace UI

/*
 * TemperaturePresenter.h
 *
 * Behaviour for the Temperature tab (assets/ui/components/control/temperature/temperature.xml): every tool with a
 * heater, with its filament (the rest of a tool, its state, extrusion factors and heaters, is read from the object
 * model, `om.tools[{row.number}]`); the beds and chambers, from the object model too; and the extruder panel
 * (temperature_extrusion.xml).
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md). Tool selection and heater states and temperatures use the global actions in
 * HeaterActions.h.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Om/OmWatch.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>
#include <vector>

namespace UI
{
	/** One tool with heaters, as the Temperature tab shows it: which tool, and the filament picked for it */
	class TemperatureToolRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Show tool `toolNumber` from the object model (the model lock held) */
		void update(size_t toolNumber);

		Xml::IntSubject number;					  // Tool number

		/** Pick `filament`, to load with change_filament */
		void pick(std::string filament);

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
		/** Follow the tools, heaters and extruders, and the filaments */
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

		/** Load `filament` in `tool`, unloading what is loaded first; unload if it is empty */
		static void changeFilament(size_t tool, const std::string& filament);

		/** Unload `tool`'s filament, if it has some */
		static void unloadFilament(size_t tool);

		Xml::List<TemperatureToolRow> m_tools;	   // Tools with heaters
		Xml::StringSubject<2048> m_filaments;	   // Known filaments, one per line
		std::vector<std::string> m_filamentNames;  // Known filaments, in the order of `m_filaments`
		Xml::StringSubject<64> m_selectedTool;	   // "Selected Tool: …"
		Xml::OmWatch m_watch;					   // Shows the tools again as the object model changes
	};
} // namespace UI

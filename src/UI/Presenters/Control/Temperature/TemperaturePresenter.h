/*
 * TemperaturePresenter.h
 *
 * Behaviour for the Temperature tab's list of tools (assets/ui/components/control/temperature/temperature.xml, the
 * `temperatures` widget): every tool with a heater, with its filament (the rest of a tool, its state, extrusion
 * factors and heaters, is read from the object model, `om.tools[{row.number}]`); the beds and chambers, from the
 * object model too; and the extruder panel (temperature_extrusion.xml).
 *
 * Its names are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md). Tool selection, heater
 * states and temperatures, and filament use the global actions in ToolActions.h, HeaterActions.h and
 * FilamentActions.h.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/Om/OmWatch.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <string>

namespace UI
{
	/** One tool with heaters, as the Temperature tab shows it: which tool, and its filament */
	class TemperatureToolRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		/** Show tool `toolNumber` from the object model (the model lock held) */
		void update(size_t toolNumber);

		Xml::IntSubject number;				 // Tool number
		Xml::IntSubject filamentShown;		 // 1 if the tool takes filament
		Xml::StringSubject<64> filamentText; // Loaded filament, or "No Filament Loaded"
		Xml::IntSubject filamentLoaded;		 // 1 if filament is loaded
	};

	class TemperaturePresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		TemperaturePresenter();

		/** Declare the tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Follow the tools, heaters and extruders */
		void onInit() override;

		/** Show the tools */
		void onActivate() override;

		/** Forget the tools */
		void onDisconnect() override;

	  private:
		/** Show every tool with heaters, and the selected tool */
		void showTools();

		Xml::List<TemperatureToolRow> m_tools; // Tools with heaters
		Xml::StringSubject<64> m_selectedTool; // "Selected Tool: …"
		Xml::OmWatch m_watch;				   // Shows the tools again as the object model changes
	};
} // namespace UI

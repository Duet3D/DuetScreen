/*
 * ToolSource.h
 *
 * Publishes the object model's tools, each with its heaters (see HeaterRow.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "HeaterRow.h"

namespace OM
{
	struct Tool;
}

namespace UI::Xml
{
	/** One heater of a tool; its `index` is its position in the tool */
	class ToolHeaterRow : public HeaterRow
	{
	};

	/** One tool */
	class ToolRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		/** Copy `tool`, and its heaters, into the subjects */
		void update(OM::Tool& tool);

		IntSubject number;			 // Tool number
		StringSubject<32> name;		 // Display name
		IntSubject selected;		 // 1 if active
		IntSubject heaterCount;		 // Number of heaters
		List<ToolHeaterRow> heaters; // The tool's heaters
	};

	/** Publishes the `tools` list */
	class ToolSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		ToolSource();

		/** Declare the `tools` list */
		void fields(FieldVisitor& f) override;

		/** Re-read every tool */
		void update() override;

		/** Empty the list */
		void clear() override;

	  protected:
		/** Listen for tool and heater changes */
		void onInit() override;

	  private:
		/** Show the current tool's heaters, as the status bar does. Call with the model locked. */
		void showCurrentTool();

		List<ToolRow> m_tools;				 // One row per tool
		IntSubject m_currentShown;			 // 1 if there is a current tool with heaters
		StringSubject<16> m_currentLabel;	 // e.g. "T0:"
		StringSubject<96> m_currentHeaters; // e.g. " 210°C/215°C 60°C/60°C"
		IntSubject m_currentFault;			 // 1 if one of its heaters has a fault
	};
} // namespace UI::Xml

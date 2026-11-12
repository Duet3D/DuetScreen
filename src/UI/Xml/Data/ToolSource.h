/*
 * ToolSource.h
 *
 * Publishes the object model's tools, each with its heaters.
 *
 *   tools             list, one row per tool in slot order
 *     index           position in the list
 *     number          tool number, as used by T
 *     name            display name
 *     selected        1 if this is the active tool, else 0
 *     heaters         list, one row per heater of the tool (fields: see HeaterRow.h)
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
		List<ToolRow> m_tools; // One row per tool
	};
} // namespace UI::Xml

/*
 * ToolSource.h
 *
 * Publishes a summary of the current tool, as the status bar shows it: its heaters' temperatures in one line, and
 * whether one has a fault; and, for the dashboard's tools widget, the tool it shows in detail and a tile per tool. The
 * tool shown follows the tool in use until someone picks another (actions.tool.show). The tools themselves are the
 * object model's, `om.tools`.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <optional>

namespace UI::Xml
{
	/** One tool, as a tile of the dashboard's tools widget */
	class ToolTileRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		IntSubject number;			// The tool's number
		StringSubject<16> label;	// E.g. "T0"
		StringSubject<64> name;		// Its name, else "Tool 0"
		StringSubject<16> state;	// "active", "standby", "off" or "fault": its heaters' (any fault marks it)
		IntSubject inUse;			// 1 if it is the tool in use
		IntSubject shown;			// 1 if it is the tool shown in detail
	};

	/** Publishes `source.tool.*` */
	class ToolSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		ToolSource();

		/** Declare the current tool's summary */
		void fields(FieldVisitor& f) override;

		/** Show the current tool again */
		void update() override;

		/** Show no current tool */
		void clear() override;

		/** Show tool `number` in detail; the tool in use, or nullopt, follows the tool in use again */
		void show(std::optional<size_t> number);

	  protected:
		/** Follow the tools, their heaters, the current tool and the sensors' names */
		void onInit() override;

	  private:
		/** Show the current tool's heaters, as the status bar does (the model lock held) */
		void showCurrentTool();

		/** Show the tool shown in detail, the tiles, and which style the tools widget picks (the model lock held) */
		void showTools();

		IntSubject m_currentShown;			 // 1 if there is a current tool with heaters
		StringSubject<16> m_currentLabel;	 // e.g. "T0:"
		StringSubject<96> m_currentHeaters; // e.g. " 210°C/215°C 60°C/60°C"
		IntSubject m_currentFault;			 // 1 if one of its heaters has a fault

		std::optional<size_t> m_picked;	 // The tool picked to show; nullopt to follow the tool in use
		IntSubject m_shown{-1};			 // `shown`: the tool shown in detail, or -1 for none
		IntSubject m_following{1};		 // `following`: 1 while the tool shown is the tool in use, followed
		IntSubject m_summaryAuto;		 // `summary_auto`: 1 if the automatic style is one tool with the rest summarised
		List<ToolTileRow> m_tiles;		 // `tiles`
	};
} // namespace UI::Xml

/*
 * ToolSource.h
 *
 * Publishes a summary of the current tool, as the status bar shows it: its heaters' temperatures in one line, and
 * whether one has a fault. The tools themselves are the object model's, `om.tools`.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI::Xml
{
	/** Publishes `source.tool.current.*` */
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

	  protected:
		/** Follow the tools, their heaters, the current tool and the sensors' names */
		void onInit() override;

	  private:
		/** Show the current tool's heaters, as the status bar does (the model lock held) */
		void showCurrentTool();

		IntSubject m_currentShown;			 // 1 if there is a current tool with heaters
		StringSubject<16> m_currentLabel;	 // e.g. "T0:"
		StringSubject<96> m_currentHeaters; // e.g. " 210°C/215°C 60°C/60°C"
		IntSubject m_currentFault;			 // 1 if one of its heaters has a fault
	};
} // namespace UI::Xml

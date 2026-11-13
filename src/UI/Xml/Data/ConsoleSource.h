/*
 * ConsoleSource.h
 *
 * Records what the console shows: commands sent from it, responses from the Duet, and log messages. It keeps
 * recording while the console isn't showing, so they are there when it is opened. Commands are recorded from the
 * ConsoleCommand event, which whatever sends them posts.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <deque>
#include <string>

enum class ResponseType;

namespace UI::Xml
{
	/** One line of the console */
	class ConsoleLineRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<24> time;		// When it arrived
		StringSubject<128> command; // A command sent, or empty
		StringSubject<1024> text;	// A response or message, or empty
	};

	class ConsoleSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		ConsoleSource();

		/** Declare `console.lines` */
		void fields(FieldVisitor& f) override;

		/** Nothing to re-read: lines are events */
		void update() override {}

		/** Keep the lines: they are the record of what happened */
		void clear() override {}

	  protected:
		/** Listen for commands, responses and log messages */
		void onInit() override;

	  private:
		/** A line to show */
		struct Line
		{
			std::string time;	 // When it arrived
			std::string command; // A command sent, or empty
			std::string text;	 // A response or message, or empty
		};

		/** Add a line, dropping the oldest beyond the limit */
		void add(Line line);

		/** Show the recorded lines */
		void publish();

		std::deque<Line> m_lines;	  // Recorded lines, oldest first
		List<ConsoleLineRow> m_rows;  // Shown lines
	};
} // namespace UI::Xml

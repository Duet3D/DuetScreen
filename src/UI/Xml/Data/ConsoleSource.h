/*
 * ConsoleSource.h
 *
 * Records what the console shows: commands sent from it, responses from the Duet, and log messages, each with its
 * kind so a line can be marked as a command, a success, a warning or an error. It keeps recording while the console
 * isn't showing, so they are there when it is opened. Commands are recorded from the ConsoleCommand event, which
 * whatever sends them posts; the latest few distinct ones are also kept as the recent commands, to send again.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <deque>
#include <string>

namespace UI::Xml
{
	/** What a line of the console is: its `kind`, which marks it */
	enum class ConsoleLineKind : int32_t
	{
		message = 0, // A response or log message with nothing to report
		command,	 // A command sent from the screen
		success,	 // A response reporting success
		warning,	 // A warning, from the Duet or the screen's log
		error,		 // An error, from the Duet or the screen's log
	};

	/** One line of the console */
	class ConsoleLineRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<16> time;	  // When it arrived, e.g. "12:34:56"
		IntSubject kind;		  // What it is: a ConsoleLineKind
		StringSubject<1024> text; // The command, response or message
	};

	/** A command sent recently, to send again */
	class ConsoleRecentRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<128> command; // The command, e.g. "G28"
	};

	class ConsoleSource : public DataSource
	{
	  public:
		/** How many recent commands are kept */
		static constexpr size_t MAX_RECENT = 6;

		/** Create the source. Nothing is published until DataSources::start(). */
		ConsoleSource();

		/** Declare `source.console.lines` and `source.console.recent` */
		void fields(FieldVisitor& f) override;

		/** Nothing to re-read: lines are events */
		void update() override {}

		/** Keep the lines and recent commands: they are the record of what happened */
		void clear() override {}

	  protected:
		/** Listen for commands, responses and log messages */
		void onInit() override;

	  private:
		/** A line to show */
		struct Line
		{
			std::string time;	  // When it arrived
			ConsoleLineKind kind; // What it is
			std::string text;	  // The command, response or message
		};

		/** Add a line, dropping the oldest beyond the limit */
		void add(Line line);

		/** Put `command` first among the recent commands, once, dropping the oldest beyond MAX_RECENT */
		void remember(const std::string& command);

		/** Show the recorded lines */
		void publishLines();

		/** Show the recent commands */
		void publishRecent();

		std::deque<Line> m_lines;			 // Recorded lines, oldest first
		std::deque<std::string> m_recent;	 // Recent distinct commands, newest first
		List<ConsoleLineRow> m_rows;		 // Shown lines
		List<ConsoleRecentRow> m_recentRows; // Shown recent commands
	};
} // namespace UI::Xml

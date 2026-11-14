/*
 * ConsoleSource.cpp
 */

#include "ConsoleSource.h"
#include "Configuration.h"
#include "Debug.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include <algorithm>
#include <chrono>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** The time now, as the console shows it, e.g. "12:34:56" */
		std::string timeNow()
		{
#if USE_FIXED_TEST_STRINGS
			return fmt::format("{:02}:{:02}:{:02}", 12, 34, 56);
#else
			const auto now = std::chrono::system_clock::now().time_since_epoch();
			return fmt::format("{:02}:{:02}:{:02}",
							   std::chrono::duration_cast<std::chrono::hours>(now).count() % 24,
							   std::chrono::duration_cast<std::chrono::minutes>(now).count() % 60,
							   std::chrono::duration_cast<std::chrono::seconds>(now).count() % 60);
#endif
		}

		/** `text` without one trailing newline: each line ends anyway */
		std::string withoutTrailingNewline(std::string text)
		{
			if (!text.empty() && text.back() == '\n')
			{
				text.pop_back();
			}
			return text;
		}

		/** The kind of line a response of `type` is */
		ConsoleLineKind kindOf(ResponseType type)
		{
			switch (type)
			{
			case ResponseType::SUCCESS:
				return ConsoleLineKind::success;
			case ResponseType::WARNING:
				return ConsoleLineKind::warning;
			case ResponseType::ERROR:
				return ConsoleLineKind::error;
			case ResponseType::INFO:
				break;
			}
			return ConsoleLineKind::message;
		}

		/** The kind of line a log message of `level` is */
		ConsoleLineKind kindOf(Log::DebugLevel level)
		{
			switch (level)
			{
			case Log::DebugLevel::Warn:
				return ConsoleLineKind::warning;
			case Log::DebugLevel::Error:
			case Log::DebugLevel::Fatal:
				return ConsoleLineKind::error;
			case Log::DebugLevel::Verbose:
			case Log::DebugLevel::Debug:
			case Log::DebugLevel::Info:
				break;
			}
			return ConsoleLineKind::message;
		}
	} // namespace

	void ConsoleLineRow::fields(FieldVisitor& f)
	{
		f.field("time", time, "When it arrived, e.g. \"12:34:56\"");
		f.field("kind",
				kind,
				"What it is: 0 a response or message, 1 a command sent, 2 a success, 3 a warning, 4 an error");
		f.field("text", text, "The command sent, e.g. \"M115\", or the response or message");
	}

	void ConsoleRecentRow::fields(FieldVisitor& f)
	{
		f.field("command", command, "The command, e.g. \"G28\"");
	}

	ConsoleSource::ConsoleSource()
		: DataSource("console")
	{
	}

	void ConsoleSource::fields(FieldVisitor& f)
	{
		f.list("lines", m_rows, "What the console shows, oldest first, at most MAX_RESPONSE_LINES");
		f.list("recent",
			   m_recentRows,
			   "The commands sent from the console lately, newest first, each once, at most ConsoleSource::MAX_RECENT");
	}

	void ConsoleSource::onInit()
	{
		registerEventListener<EventType::ConsoleCommand>(
			[this](const std::string& command)
			{
				add({timeNow(), ConsoleLineKind::command, command});
				remember(command);
			});
		registerEventListener<EventType::Response>(
			[this](ResponseType type, const std::string& response)
			{ add({timeNow(), kindOf(type), withoutTrailingNewline(response)}); });
		registerEventListener<EventType::LogMessage>(
			[this](const Log::DebugLevel& level, const Log::log_time_t&, const std::string& message)
			{ add({timeNow(), kindOf(level), withoutTrailingNewline(message)}); });
	}

	void ConsoleSource::add(Line line)
	{
		m_lines.push_back(std::move(line));
		while (m_lines.size() > MAX_RESPONSE_LINES)
		{
			m_lines.pop_front();
		}
		publishLines();
	}

	void ConsoleSource::remember(const std::string& command)
	{
		std::erase(m_recent, command);
		m_recent.push_front(command);
		while (m_recent.size() > MAX_RECENT)
		{
			m_recent.pop_back();
		}
		publishRecent();
	}

	void ConsoleSource::publishLines()
	{
		ZoneScoped;
		UI_LOCK();
		m_rows.sync(m_lines.size(),
					[this](ConsoleLineRow& row, size_t i)
					{
						const Line& line = m_lines[i];
						row.time.set(line.time);
						row.kind.set(static_cast<int32_t>(line.kind));
						row.text.set(line.text);
					});
	}

	void ConsoleSource::publishRecent()
	{
		ZoneScoped;
		UI_LOCK();
		m_recentRows.sync(m_recent.size(), [this](ConsoleRecentRow& row, size_t i) { row.command.set(m_recent[i]); });
	}
} // namespace UI::Xml

/*
 * ConsoleSource.cpp
 */

#include "ConsoleSource.h"
#include "Configuration.h"
#include "Debug.h"
#include "Subscribers/ResponseSubscribers.h"
#include "UI/Core/Model.h"
#include <chrono>
#include <fmt/format.h>

namespace UI::Xml
{
	namespace
	{
		/** The time now, as the console shows it */
		std::string timeNow()
		{
#if USE_FIXED_TEST_STRINGS
			return fmt::format("[{:02}:{:02}:{:02}]   ", 12, 34, 56);
#else
			const auto now = std::chrono::system_clock::now().time_since_epoch();
			return fmt::format("[{:02}:{:02}:{:02}]   ",
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
	} // namespace

	void ConsoleLineRow::fields(FieldVisitor& f)
	{
		f.field("time", time, "When it arrived, e.g. \"[12:34:56]   \"");
		f.field("command", command, "A command sent, e.g. \"> M115\"; empty for a response");
		f.field("text", text, "A response or log message; empty for a command");
	}

	ConsoleSource::ConsoleSource()
		: DataSource("console")
	{
	}

	void ConsoleSource::fields(FieldVisitor& f)
	{
		f.list("console.lines", m_rows, "What the console shows, oldest first, at most MAX_RESPONSE_LINES");
	}

	void ConsoleSource::onInit()
	{
		registerEventListener<EventType::ConsoleCommand>([this](const std::string& command)
														 { add({timeNow(), fmt::format("> {:s}", command), ""}); });
		registerEventListener<EventType::Response>([this](ResponseType, const std::string& response)
												   { add({timeNow(), "", withoutTrailingNewline(response)}); });
		registerEventListener<EventType::LogMessage>(
			[this](const Log::DebugLevel&, const Log::log_time_t&, const std::string& message)
			{ add({timeNow(), "", withoutTrailingNewline(message)}); });
	}

	void ConsoleSource::add(Line line)
	{
		m_lines.push_back(std::move(line));
		while (m_lines.size() > MAX_RESPONSE_LINES)
		{
			m_lines.pop_front();
		}
		publish();
	}

	void ConsoleSource::publish()
	{
		ZoneScoped;
		UI_LOCK();
		m_rows.sync(m_lines.size(),
					[this](ConsoleLineRow& row, size_t i)
					{
						const Line& line = m_lines[i];
						row.time.set(line.time);
						row.command.set(line.command);
						row.text.set(line.text);
					});
	}
} // namespace UI::Xml

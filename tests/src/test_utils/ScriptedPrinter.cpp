/*
 * ScriptedPrinter.cpp
 */

#include "ScriptedPrinter.h"
#include "Comm/Connection.h"
#include <charconv>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace
{
	/** A line sent split into its number and its command */
	struct Numbered
	{
		int64_t number = -1;	  // N, or -1 if it has none
		std::string_view command; // What follows it
	};

	/** Split `line` ("N12 M409 …") into its number and command */
	Numbered split(std::string_view line)
	{
		Numbered numbered{.command = line};
		if (!line.starts_with('N'))
		{
			return numbered;
		}
		const char* first = line.data() + 1;
		const auto [end, error] = std::from_chars(first, line.data() + line.size(), numbered.number);
		if (error != std::errc())
		{
			numbered.number = -1;
			return numbered;
		}
		numbered.command = line.substr(static_cast<size_t>(end - line.data()));
		while (numbered.command.starts_with(' '))
		{
			numbered.command.remove_prefix(1);
		}
		return numbered;
	}
} // namespace

bool ScriptedPrinter::open(Receiver receiver)
{
	m_receiver = std::move(receiver);
	return true;
}

void ScriptedPrinter::close()
{
	m_receiver = nullptr;
}

bool ScriptedPrinter::write(size_t, std::string_view bytes)
{
	while (bytes.ends_with('\n'))
	{
		bytes.remove_suffix(1);
	}
	m_lines.emplace_back(bytes);
	return true;
}

bool ScriptedPrinter::answer(std::string_view command, std::string_view reply)
{
	Comm::Connection::get().process();
	for (size_t i = 0; i < m_lines.size(); i++)
	{
		const Numbered line = split(m_lines[i]);
		if (m_answered.contains(i) || !line.command.starts_with(command))
		{
			continue;
		}
		nlohmann::ordered_json json = nlohmann::ordered_json::parse(reply);
		json["line"] = line.number;
		m_answered.insert(i);
		EXPECT_TRUE(m_receiver) << "the port is closed";
		if (m_receiver)
		{
			m_receiver(0, json.dump() + "\n");
		}
		Comm::Connection::get().process();
		return true;
	}
	return false;
}

std::vector<std::string> ScriptedPrinter::sent() const
{
	std::vector<std::string> commands;
	for (const std::string& line : m_lines)
	{
		commands.emplace_back(split(line).command);
	}
	return commands;
}

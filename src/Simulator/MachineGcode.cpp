/*
 * MachineGcode.cpp
 *
 * The G-code the simulated machine understands: what the screen sends, and what a developer is likely to type in its
 * console. Anything else is accepted and ignored, as long as it parses.
 */

#include "Debug.h"
#include "Machine.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fmt/format.h>

namespace Sim
{
	/** A G, M or T code with its parameters */
	struct Machine::Command
	{
		char letter = 0;					// 'G', 'M' or 'T'
		int32_t code = 0;					// Its number, e.g. 36 for M36.1; for T the tool (-1 for T-1)
		int32_t fraction = -1;				// What follows the point, e.g. 1 for M36.1; -1 if none
		bool bareT = false;					// A T without a number: which tool is in use?
		std::map<char, std::string> params; // Its parameters by letter, quoted strings unquoted; '@' for a bare string
		std::string rest;					// Everything after the code, trimmed, e.g. a file name

		/** Whether it is `letter` `code` (and `.fraction`, if given) */
		bool is(char l, int32_t c, int32_t f = -1) const { return letter == l && code == c && fraction == f; }

		/** Whether it has parameter `p` */
		bool has(char p) const { return params.contains(p); }

		/** Parameter `p` as a number, if it is one */
		std::optional<float> number(char p) const
		{
			const auto it = params.find(p);
			if (it == params.end() || it->second.empty())
			{
				return std::nullopt;
			}
			float value = 0.0f;
			const std::string& text = it->second;
			const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
			if (error != std::errc())
			{
				return std::nullopt;
			}
			return value;
		}

		/** Parameter `p` as a whole number, if it is a number */
		std::optional<int32_t> integer(char p) const
		{
			const auto value = number(p);
			return value ? std::optional<int32_t>(static_cast<int32_t>(std::lround(*value))) : std::nullopt;
		}

		/** Parameter `p` as an exact whole number of 32 bits, e.g. a size or a CRC, if it is one */
		std::optional<uint32_t> unsignedInteger(char p) const
		{
			const auto it = params.find(p);
			if (it == params.end())
			{
				return std::nullopt;
			}
			uint32_t value = 0;
			const std::string& text = it->second;
			const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
			return error == std::errc() && end == text.data() + text.size() ? std::optional(value) : std::nullopt;
		}

		/** Parameter `p` as numbers separated by colons, e.g. G10's S210:200 */
		std::vector<float> numbers(char p) const
		{
			std::vector<float> values;
			const auto it = params.find(p);
			if (it == params.end())
			{
				return values;
			}
			std::string_view text = it->second;
			while (!text.empty())
			{
				const size_t colon = text.find(':');
				const std::string_view part = text.substr(0, colon);
				float value = 0.0f;
				if (std::from_chars(part.data(), part.data() + part.size(), value).ec == std::errc())
				{
					values.push_back(value);
				}
				if (colon == std::string_view::npos)
				{
					break;
				}
				text.remove_prefix(colon + 1);
			}
			return values;
		}

		/** Parameter `p`'s text, or "" */
		std::string text(char p) const
		{
			const auto it = params.find(p);
			return it != params.end() ? it->second : std::string();
		}

		/** The file it names: a bare quoted string, else everything after the code without quotes */
		std::string file() const
		{
			if (has('@'))
			{
				return text('@');
			}
			std::string name = rest;
			if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
			{
				name = name.substr(1, name.size() - 2);
			}
			return name;
		}
	};

	namespace
	{
		/** Seconds probing the bed takes, and levelling it */
		constexpr double PROBE_TIME = 2.0;

		/** `text` without leading and trailing spaces */
		std::string_view trim(std::string_view text)
		{
			while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
			{
				text.remove_prefix(1);
			}
			while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
			{
				text.remove_suffix(1);
			}
			return text;
		}

		/** `line` without its line number (N123), checksum (*45) and comment (; …) */
		std::string_view strip(std::string_view line)
		{
			line = trim(line);
			if (!line.empty() && (line.front() == 'N' || line.front() == 'n'))
			{
				size_t i = 1;
				while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i])))
				{
					i++;
				}
				if (i > 1)
				{
					line.remove_prefix(i);
				}
			}
			bool quoted = false;
			for (size_t i = 0; i < line.size(); i++)
			{
				if (line[i] == '"')
				{
					quoted = !quoted;
				}
				else if (!quoted && (line[i] == ';' || line[i] == '*'))
				{
					line = line.substr(0, i);
					break;
				}
			}
			return trim(line);
		}

		/**
		 * `line` split into its commands: a new one starts at each G or M word (a letter then a digit after a space),
		 * e.g. "M112 M999" or "G53 G1 X10". T starts one only at the start, as T is also a parameter.
		 */
		std::vector<std::string_view> splitCommands(std::string_view line)
		{
			std::vector<std::string_view> commands;
			size_t start = 0;
			bool quoted = false;
			int braces = 0;
			for (size_t i = 1; i + 1 < line.size(); i++)
			{
				const char c = line[i];
				if (c == '"')
				{
					quoted = !quoted;
				}
				else if (!quoted && c == '{')
				{
					braces++;
				}
				else if (!quoted && c == '}')
				{
					braces--;
				}
				const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
				if (!quoted && braces == 0 && (upper == 'G' || upper == 'M') && line[i - 1] == ' ' &&
					std::isdigit(static_cast<unsigned char>(line[i + 1])))
				{
					commands.push_back(trim(line.substr(start, i - start)));
					start = i;
				}
			}
			commands.push_back(trim(line.substr(start)));
			return commands;
		}

		/** The message box's choices from M291 K{"a","b"}: each quoted string */
		std::vector<std::string> choicesOf(std::string_view text)
		{
			std::vector<std::string> choices;
			size_t i = 0;
			while ((i = text.find('"', i)) != std::string_view::npos)
			{
				const size_t end = text.find('"', i + 1);
				if (end == std::string_view::npos)
				{
					break;
				}
				choices.emplace_back(text.substr(i + 1, end - i - 1));
				i = end + 1;
			}
			return choices;
		}

		/** The workplace (0-8) G54 to G59.3 select, or -1 */
		int32_t workplaceOf(int32_t code, int32_t fraction)
		{
			if (code >= 54 && code <= 58 && fraction < 0)
			{
				return code - 54;
			}
			if (code == 59)
			{
				return fraction < 0 ? 5 : 5 + fraction;
			}
			return -1;
		}
	} // namespace

	std::optional<Machine::Command> Machine::parse(std::string_view text)
	{
		text = trim(text);
		if (text.empty())
		{
			return std::nullopt;
		}
		Command command;
		command.letter = static_cast<char>(std::toupper(static_cast<unsigned char>(text.front())));
		if (command.letter != 'G' && command.letter != 'M' && command.letter != 'T')
		{
			return std::nullopt;
		}
		size_t i = 1;
		const size_t numberStart = i;
		if (i < text.size() && text[i] == '-')
		{
			i++;
		}
		while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
		{
			i++;
		}
		if (i == numberStart)
		{
			if (command.letter != 'T')
			{
				return std::nullopt;
			}
			command.bareT = true;
		}
		else
		{
			std::from_chars(text.data() + numberStart, text.data() + i, command.code);
		}
		if (i < text.size() && text[i] == '.')
		{
			const size_t fractionStart = ++i;
			while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])))
			{
				i++;
			}
			std::from_chars(text.data() + fractionStart, text.data() + i, command.fraction);
		}
		const std::string_view rest = trim(text.substr(i));
		command.rest = std::string(rest);

		// Parameters: a letter then a value, a quoted string or an {expression}
		size_t p = 0;
		while (p < rest.size())
		{
			char c = rest[p];
			char key = 0;
			if (std::isalpha(static_cast<unsigned char>(c)))
			{
				key = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
				p++;
			}
			else if (c == '"')
			{
				key = '@';
			}
			else
			{
				p++;
				continue;
			}
			std::string value;
			if (p < rest.size() && rest[p] == '"')
			{
				p++;
				while (p < rest.size())
				{
					if (rest[p] == '"')
					{
						if (p + 1 < rest.size() && rest[p + 1] == '"')
						{
							value += '"';
							p += 2;
							continue;
						}
						p++;
						break;
					}
					value += rest[p++];
				}
			}
			else if (p < rest.size() && rest[p] == '{')
			{
				int depth = 0;
				const size_t start = p;
				for (; p < rest.size(); p++)
				{
					depth += rest[p] == '{' ? 1 : rest[p] == '}' ? -1 : 0;
					if (depth == 0)
					{
						p++;
						break;
					}
				}
				value = std::string(rest.substr(start + 1, p - start - 2));
			}
			else
			{
				while (p < rest.size() && !std::isspace(static_cast<unsigned char>(rest[p])))
				{
					value += rest[p++];
				}
			}
			command.params[key] = value;
		}
		return command;
	}

	std::vector<Json> Machine::execute(std::string_view gcode)
	{
		std::vector<Json> replies;
		std::string reply;
		size_t start = 0;
		while (start <= gcode.size())
		{
			const size_t end = std::min(gcode.find('\n', start), gcode.size());
			executeLine(gcode.substr(start, end - start), reply, replies);
			start = end + 1;
		}
		if (!reply.empty())
		{
			while (!reply.empty() && reply.back() == '\n')
			{
				reply.pop_back();
			}
			m_replySeq++;
			replies.push_back(Json{{"resp", reply}});
		}
		return replies;
	}

	void Machine::executeLine(std::string_view line, std::string& reply, std::vector<Json>& replies)
	{
		const std::string_view stripped = strip(line);
		if (stripped.empty())
		{
			return;
		}
		bool machineCoordinates = false;
		for (const std::string_view text : splitCommands(stripped))
		{
			const auto command = parse(text);
			if (!command)
			{
				continue;
			}
			if (command->is('G', 53))
			{
				machineCoordinates = true;
				continue;
			}
			// Halted, only a reset, an emergency stop or a question about the machine is answered
			if (m_halted && !command->is('M', 999) && !command->is('M', 112) && !command->is('M', 409))
			{
				reply += "Error: the machine is halted: send M999 to reset it\n";
				continue;
			}
			if (machineCoordinates && command->letter == 'G' && command->code <= 1)
			{
				const int32_t workplace = m_workplace;
				m_workplace = -1; // no offsets
				run(*command, reply, replies);
				m_workplace = workplace;
				continue;
			}
			run(*command, reply, replies);
		}
	}

	void Machine::run(const Command& command, std::string& reply, std::vector<Json>& replies)
	{
		const auto currentToolOr = [this](std::optional<int32_t> tool) -> int32_t
		{ return tool.value_or(m_currentTool >= 0 ? m_currentTool : 0); };

		if (command.letter == 'T')
		{
			if (command.bareT)
			{
				reply += m_currentTool >= 0 ? fmt::format("Tool {:d} is selected.\n", m_currentTool)
											: "No tool is selected.\n";
			}
			else if (command.code >= static_cast<int32_t>(m_tools.size()))
			{
				reply += fmt::format("Error: invalid tool number {:d}\n", command.code);
			}
			else
			{
				selectTool(command.code);
			}
			return;
		}

		if (command.letter == 'G')
		{
			if (const int32_t workplace = workplaceOf(command.code, command.fraction); workplace >= 0)
			{
				m_workplace = workplace;
				return;
			}
			switch (command.code)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				move(command, reply);
				break;
			case 10:
				if (command.integer('L') == 20 || command.integer('L') == 2)
				{
					// Workplace offsets: L2 sets them, L20 makes the current position the value given
					const int32_t workplace = command.integer('P').value_or(m_workplace + 1) - 1;
					if (workplace < 0 || workplace >= 9)
					{
						reply += "Error: G10: invalid workplace\n";
						break;
					}
					for (size_t i = 0; i < m_axes.size(); i++)
					{
						const auto value = command.number(m_config.axes[i].letter.front());
						if (value)
						{
							m_axes[i].workplaceOffsets[static_cast<size_t>(workplace)] =
								command.integer('L') == 20 ? m_axes[i].position - *value : *value;
						}
					}
				}
				else if (command.has('P') && (command.has('S') || command.has('R')))
				{
					// A tool's heaters' temperatures
					const int32_t tool = *command.integer('P');
					if (tool < 0 || tool >= static_cast<int32_t>(m_tools.size()))
					{
						reply += fmt::format("Error: G10: invalid tool {:d}\n", tool);
						break;
					}
					const auto active = command.numbers('S');
					const auto standby = command.numbers('R');
					const auto& heaters = m_config.tools[static_cast<size_t>(tool)].heaters;
					for (size_t h = 0; h < heaters.size(); h++)
					{
						const auto pick = [h](const std::vector<float>& values) -> std::optional<float>
						{
							if (values.empty())
							{
								return std::nullopt;
							}
							return values[std::min(h, values.size() - 1)];
						};
						setHeaterTemperature(static_cast<size_t>(heaters[h]), pick(active), pick(standby), reply);
					}
				}
				break;
			case 29:
			case 30:
			case 32:
				probe(command, reply);
				break;
			case 28:
			{
				// Home the axes named, or all of them
				bool any = false;
				for (const Axis& axis : m_config.axes)
				{
					any = any || command.has(axis.letter.front());
				}
				for (size_t i = 0; i < m_axes.size(); i++)
				{
					if (!any || command.has(m_config.axes[i].letter.front()))
					{
						m_axes[i].position = m_axes[i].target = m_config.axes[i].min;
						m_axes[i].homed = true;
					}
				}
				m_transition = Transition{.status = "busy", .remaining = 1.0, .then = ""};
				break;
			}
			case 90:
				m_relative = false;
				break;
			case 91:
				m_relative = true;
				break;
			case 92:
				for (size_t i = 0; i < m_axes.size(); i++)
				{
					if (const auto value = command.number(m_config.axes[i].letter.front()))
					{
						const float offset =
							m_workplace >= 0 ? m_axes[i].workplaceOffsets[static_cast<size_t>(m_workplace)] : 0.0f;
						m_axes[i].position = m_axes[i].target = *value + offset;
					}
				}
				break;
			default:
				LOG_DBG("Simulated Duet: G{:d} ignored", command.code);
				break;
			}
			return;
		}

		// M codes
		switch (command.code)
		{
		case 0:
		case 1:
			if (m_job.running)
			{
				m_transition = Transition{.status = "cancelling", .remaining = 1.0, .then = "cancelled"};
				stopEverything();
			}
			break;
		case 3:
		case 4:
		case 5:
		{
			int32_t spindle = command.integer('P').value_or(-1);
			if (spindle < 0 && m_currentTool >= 0)
			{
				spindle = m_config.tools[static_cast<size_t>(m_currentTool)].spindle;
			}
			if (spindle < 0 || spindle >= static_cast<int32_t>(m_spindles.size()))
			{
				reply += fmt::format("Error: M{:d}: no spindle\n", command.code);
				break;
			}
			SpindleState& state = m_spindles[static_cast<size_t>(spindle)];
			if (command.code == 5)
			{
				state.state = "stopped";
				break;
			}
			if (command.code == 4 && !m_config.spindles[static_cast<size_t>(spindle)].canReverse)
			{
				reply += "Error: M4: the spindle can't turn in reverse\n";
				break;
			}
			const int32_t toolRpm = m_currentTool >= 0 ? m_tools[static_cast<size_t>(m_currentTool)].spindleRpm : 0;
			state.active =
				std::min(command.integer('S').value_or(toolRpm), m_config.spindles[static_cast<size_t>(spindle)].max);
			state.state = command.code == 3 ? "forward" : "reverse";
			break;
		}
		case 18:
		case 84:
		{
			bool any = false;
			for (const Axis& axis : m_config.axes)
			{
				any = any || command.has(axis.letter.front());
			}
			for (size_t i = 0; i < m_axes.size(); i++)
			{
				if (!any || command.has(m_config.axes[i].letter.front()))
				{
					m_axes[i].homed = false;
				}
			}
			break;
		}
		case 20:
			// RepRapFirmware refuses to list a folder that isn't named
			if (command.has('P') && command.text('P').empty())
			{
				reply += "Error: M20: expected a non-empty string\n";
			}
			else
			{
				replies.push_back(fileListReply(command.text('P'), command.integer('R').value_or(0)));
			}
			break;
		case 23:
			if (const JobFile* file = findFile(command.file()))
			{
				m_job.file = static_cast<size_t>(file - m_config.files.data());
				reply += fmt::format("File {:s} selected for printing\n", file->name);
			}
			else
			{
				reply += fmt::format("Error: M23: can't find file {:s}\n", command.file());
			}
			break;
		case 24:
			if (m_job.running && m_transition && m_transition->status == "paused")
			{
				m_transition = Transition{.status = "resuming", .remaining = TRANSITION_TIME, .then = ""};
			}
			else if (!m_job.running && m_job.file < m_config.files.size())
			{
				startJob(m_config.files[m_job.file].name, reply);
			}
			break;
		case 25:
		case 226:
			if (m_job.running && !m_transition)
			{
				m_transition = Transition{.status = "pausing", .remaining = TRANSITION_TIME, .then = "paused"};
			}
			break;
		case 30:
			if (const JobFile* file = findFile(command.file()))
			{
				m_config.files.erase(m_config.files.begin() + (file - m_config.files.data()));
				m_configChanged = true;
			}
			break;
		case 32:
			startJob(command.file(), reply);
			break;
		case 36:
			if (command.fraction == 1)
			{
				replies.push_back(thumbnailReply(command.text('P'),
												 static_cast<uint32_t>(std::max(0, command.integer('S').value_or(0)))));
			}
			else if (command.fraction == 2)
			{
				replies.push_back(
					fileContentsReply(command.text('P'), static_cast<uint32_t>(command.integer('S').value_or(0))));
			}
			else
			{
				replies.push_back(fileInfoReply(command.file()));
			}
			break;
		case 104:
		case 109:
		{
			const int32_t tool = currentToolOr(command.integer('T'));
			if (tool >= static_cast<int32_t>(m_tools.size()))
			{
				reply += fmt::format("Error: M{:d}: invalid tool {:d}\n", command.code, tool);
				break;
			}
			for (const int heater : m_config.tools[static_cast<size_t>(tool)].heaters)
			{
				setHeaterTemperature(static_cast<size_t>(heater), command.number('S'), command.number('R'), reply);
			}
			break;
		}
		case 106:
		case 107:
		{
			int32_t fan = command.integer('P').value_or(-1);
			if (fan < 0)
			{
				const auto& fans =
					m_currentTool >= 0 ? m_config.tools[static_cast<size_t>(m_currentTool)].fans : std::vector<int>{0};
				fan = fans.empty() ? 0 : fans.front();
			}
			if (fan >= static_cast<int32_t>(m_fans.size()))
			{
				reply += fmt::format("Error: M{:d}: no fan {:d}\n", command.code, fan);
				break;
			}
			if (m_config.fans[static_cast<size_t>(fan)].thermostatic)
			{
				break; // it follows its heater
			}
			float speed = command.code == 107 ? 0.0f : command.number('S').value_or(1.0f);
			speed = speed > 1.0f ? speed / 255.0f : speed;
			m_fans[static_cast<size_t>(fan)].requested = std::clamp(speed, 0.0f, 1.0f);
			break;
		}
		case 112:
			m_halted = true;
			stopEverything();
			endJob();
			break;
		case 115:
			reply += "FIRMWARE_NAME: RepRapFirmware for Duet 3 (simulated) FIRMWARE_VERSION: 3.6.0 "
					 "ELECTRONICS: Simulated Duet FIRMWARE_DATE: 2026-11-15\n";
			break;
		case 117:
			m_displayMessage = command.file();
			break;
		case 120:
			m_pushedRelative.push_back(m_relative);
			break;
		case 121:
			if (!m_pushedRelative.empty())
			{
				m_relative = m_pushedRelative.back();
				m_pushedRelative.pop_back();
			}
			break;
		case 122:
			reply += "=== Diagnostics ===\nThe simulated Duet has nothing to report\n";
			break;
		case 144:
		{
			// A bed on standby (S0, or none), or active again (S1)
			const auto bed = bedOrChamberHeater(true, command.integer('P').value_or(0));
			if (!bed)
			{
				reply += "Error: M144: no such bed heater\n";
				break;
			}
			if (m_heaters[*bed].state != "fault")
			{
				m_heaters[*bed].state = command.integer('S').value_or(0) == 1 ? "active" : "standby";
			}
			break;
		}
		case 562:
			// Reset a heater's fault (all of them without P): it is off, until it faults again
			for (size_t h = 0; h < m_heaters.size(); h++)
			{
				if ((!command.has('P') || command.integer('P') == static_cast<int32_t>(h)) &&
					m_heaters[h].state == "fault")
				{
					m_heaters[h].state = "off";
				}
			}
			break;
		case 140:
		case 141:
		case 190:
		case 191:
		{
			const bool bed = command.code == 140 || command.code == 190;
			const int32_t index = command.integer('P').value_or(0);
			const auto heaterNumber = bedOrChamberHeater(bed, index);
			if (!heaterNumber)
			{
				reply +=
					fmt::format("Error: M{:d}: no {:s} heater {:d}\n", command.code, bed ? "bed" : "chamber", index);
				break;
			}
			const size_t found = *heaterNumber;
			HeaterState& heater = m_heaters[found];
			if (command.number('S') && *command.number('S') < 0.0f)
			{
				if (heater.state != "fault")
				{
					heater.state = "off";
				}
				break;
			}
			setHeaterTemperature(found, command.number('S'), command.number('R'), reply);
			if (heater.state == "off" && command.has('S'))
			{
				heater.state = "active";
			}
			break;
		}
		case 220:
			if (const auto percent = command.number('S'))
			{
				m_speedFactor = std::max(0.01f, *percent / 100.0f);
			}
			else
			{
				reply += fmt::format("Speed factor: {:.0f}%\n", m_speedFactor * 100.0f);
			}
			break;
		case 221:
			if (const auto percent = command.number('S'))
			{
				const int32_t extruder = command.integer('D').value_or(-1);
				for (size_t e = 0; e < m_extruders.size(); e++)
				{
					if (extruder < 0 || static_cast<size_t>(extruder) == e)
					{
						m_extruders[e].factor = std::max(0.0f, *percent / 100.0f);
					}
				}
			}
			break;
		case 290:
		{
			const auto amount = command.has('Z') ? command.number('Z') : command.number('S');
			for (size_t i = 0; i < m_axes.size(); i++)
			{
				if (m_config.axes[i].letter == "Z" && amount)
				{
					m_axes[i].babystep = command.integer('R') == 0 ? *amount : m_axes[i].babystep + *amount;
				}
			}
			break;
		}
		case 291:
		{
			MessageBox box{.seq = ++m_messageBoxSeq,
						   .mode = command.integer('S').value_or(1),
						   .title = command.text('R'),
						   .message = command.text('P'),
						   .cancelButton = command.integer('J').value_or(0) != 0,
						   .timeout = command.number('T').value_or(0.0f)};
			for (size_t i = 0; i < m_axes.size(); i++)
			{
				if (command.has(m_config.axes[i].letter.front()))
				{
					box.axisControls |= 1 << i;
				}
			}
			if (command.has('K'))
			{
				box.choices = choicesOf(command.text('K'));
			}
			box.min = command.number('L');
			box.max = command.number('H');
			if (command.has('F'))
			{
				if (const auto number = command.number('F'))
				{
					box.defaultValue = *number;
				}
				else
				{
					box.defaultValue = command.text('F');
				}
			}
			if (box.mode <= 1 && box.timeout == 0.0f)
			{
				box.timeout = box.mode == 0 ? 10.0f : 0.0f;
			}
			m_messageBox = box;
			break;
		}
		case 292:
			if (m_messageBox && (!command.has('S') || command.integer('S') == m_messageBox->seq))
			{
				m_messageBox.reset();
			}
			break;
		case 409:
			replies.push_back(modelReply(command.text('K'), command.has('F') ? command.text('F') : "v"));
			break;
		case 486:
			if (const auto object = command.integer('P'); object && *object >= 0 && m_job.running)
			{
				if (static_cast<size_t>(*object) < m_job.cancelled.size())
				{
					m_job.cancelled[static_cast<size_t>(*object)] = true;
				}
			}
			break;
		case 568:
		{
			const int32_t tool = currentToolOr(command.integer('P'));
			if (tool >= static_cast<int32_t>(m_tools.size()))
			{
				reply += fmt::format("Error: M568: invalid tool {:d}\n", tool);
				break;
			}
			const auto active = command.numbers('S');
			const auto standby = command.numbers('R');
			const auto& heaters = m_config.tools[static_cast<size_t>(tool)].heaters;
			for (size_t h = 0; h < heaters.size(); h++)
			{
				const auto pick = [h](const std::vector<float>& values) -> std::optional<float>
				{
					return values.empty() ? std::nullopt : std::optional<float>(values[std::min(h, values.size() - 1)]);
				};
				setHeaterTemperature(static_cast<size_t>(heaters[h]), pick(active), pick(standby), reply);
			}
			if (const auto rpm = command.integer('F'))
			{
				m_tools[static_cast<size_t>(tool)].spindleRpm = *rpm;
				const int32_t spindle = m_config.tools[static_cast<size_t>(tool)].spindle;
				if (spindle >= 0 && spindle < static_cast<int32_t>(m_spindles.size()) &&
					m_spindles[static_cast<size_t>(spindle)].state != "stopped")
				{
					m_spindles[static_cast<size_t>(spindle)].active = *rpm;
				}
			}
			if (const auto state = command.integer('A'))
			{
				// The tool's heaters, as the tool: off, standby or active
				static constexpr const char* states[] = {"off", "standby", "active"};
				const char* chosen = states[std::clamp(*state, 0, 2)];
				m_tools[static_cast<size_t>(tool)].state = chosen;
				for (const int heater : heaters)
				{
					if (heater >= 0 && static_cast<size_t>(heater) < m_heaters.size() &&
						m_heaters[static_cast<size_t>(heater)].state != "fault")
					{
						m_heaters[static_cast<size_t>(heater)].state = chosen;
					}
				}
			}
			break;
		}
		case 701:
		case 702:
			if (const auto extruder = toolExtruder())
			{
				m_extruders[*extruder].filament = command.code == 701 ? command.text('S') : "";
			}
			else
			{
				reply += fmt::format("Error: M{:d}: no tool with an extruder is selected\n", command.code);
			}
			break;
		case 559:
			startFile(command, reply);
			break;
		case 999:
		{
			// A reset: the machine starts again, cold, unhomed and idle
			const Config config = m_config;
			const auto seqs = m_seqs;
			const int32_t replySeq = m_replySeq;
			const int32_t boxSeq = m_messageBoxSeq;
			const float noise = m_noise;
			auto writtenFiles = std::move(m_writtenFiles); // on its SD card
			*this = Machine(config);
			m_writtenFiles = std::move(writtenFiles);
			m_seqs = seqs;
			m_replySeq = replySeq;
			m_messageBoxSeq = boxSeq;
			m_noise = noise;
			for (AxisState& axis : m_axes)
			{
				axis.homed = false;
			}
			break;
		}
		default:
			LOG_DBG("Simulated Duet: M{:d} ignored", command.code);
			break;
		}
	}

	void Machine::startFile(const Command& command, std::string& reply)
	{
		if (command.text('P').empty())
		{
			reply += "Error: M559: missing parameter 'P'\n";
			return;
		}
		m_upload = Upload{.path = sdPath(command.text('P')),
						  .size = command.unsignedInteger('S').value_or(0),
						  .crc = command.unsignedInteger('C'),
						  .contents = {}};
		if (m_upload->crc == 0u)
		{
			m_upload->crc.reset(); // as RepRapFirmware takes C0: no CRC
		}
		reply += fmt::format("Writing to file: {:s}\n", command.text('P'));
	}

	void Machine::move(const Command& command, std::string& reply)
	{
		std::vector<float> targets;
		targets.reserve(m_axes.size());
		for (size_t i = 0; i < m_axes.size(); i++)
		{
			AxisState& axis = m_axes[i];
			const Axis& limits = m_config.axes[i];
			const auto value = command.number(limits.letter.front());
			float target = axis.target;
			if (value)
			{
				if (!axis.homed)
				{
					reply += "Error: G0/G1: insufficient axes homed\n";
					return;
				}
				const float offset = m_workplace >= 0 ? axis.workplaceOffsets[static_cast<size_t>(m_workplace)] : 0.0f;
				target = m_relative ? axis.target + *value : *value + offset;
			}
			targets.push_back(std::clamp(target, limits.min, std::max(limits.min, limits.max)));
		}
		for (size_t i = 0; i < m_axes.size(); i++)
		{
			m_axes[i].target = targets[i];
		}
		if (const auto feed = command.number('F'))
		{
			m_feedRate = std::max(0.1f, *feed / 60.0f);
		}
		const auto extruder = toolExtruder();
		if (const auto extrude = command.number('E'); extrude && extruder)
		{
			ExtruderState& state = m_extruders[*extruder];
			state.position = m_relative || m_relativeExtrusion ? state.position + *extrude * state.factor : *extrude;
		}
	}

	void Machine::probe(const Command& command, std::string& reply)
	{
		if (command.code == 29 && command.integer('S').value_or(0) == 2)
		{
			m_heightmap.clear();
			return;
		}
		if (command.code == 29 && command.integer('S') == 1)
		{
			std::string name = command.has('P') ? command.text('P') : std::string(heightmapNames().front());
			const size_t slash = name.find_last_of('/');
			name = slash == std::string::npos ? name : name.substr(slash + 1);
			if (std::ranges::find(heightmapNames(), name) == heightmapNames().end())
			{
				reply += fmt::format("Error: G29: height map file {:s} not found\n", name);
				return;
			}
			m_heightmap = name;
			return;
		}
		// Probing, or levelling (G32): X, Y and Z must be homed
		for (size_t a = 0; a < std::min<size_t>(3, m_axes.size()); a++)
		{
			if (!m_axes[a].homed)
			{
				reply += fmt::format("Error: G{:d}: insufficient axes homed\n", command.code);
				return;
			}
		}
		if (command.code == 29)
		{
			m_probes++;
			m_heightmap = heightmapNames().front();
		}
		m_transition =
			Transition{.status = "busy", .remaining = command.code == 30 ? PROBE_TIME / 4 : PROBE_TIME, .then = ""};
	}

	std::optional<size_t> Machine::bedOrChamberHeater(bool bed, int32_t index) const
	{
		int32_t count = 0;
		for (size_t h = 0; h < m_config.heaters.size(); h++)
		{
			if (m_config.heaters[h].role == (bed ? HeaterRole::bed : HeaterRole::chamber) && count++ == index)
			{
				return h;
			}
		}
		return std::nullopt;
	}

	void Machine::setHeaterTemperature(size_t heater,
									   std::optional<float> active,
									   std::optional<float> standby,
									   std::string& reply)
	{
		if (heater >= m_heaters.size())
		{
			reply += fmt::format("Error: heater {:d} doesn't exist\n", heater);
			return;
		}
		const float max = m_config.heaters[heater].max;
		for (const auto& value : {active, standby})
		{
			if (value && *value > max)
			{
				reply += fmt::format("Error: heater {:d}: temperature {:.0f}°C is higher than its limit of {:.0f}°C\n",
									 heater,
									 *value,
									 max);
				return;
			}
		}
		if (active)
		{
			m_heaters[heater].active = *active;
		}
		if (standby)
		{
			m_heaters[heater].standby = *standby;
		}
		// The tool in use's heaters, and the bed and chamber, heat as soon as they are given a temperature
		const bool inCurrentTool =
			m_currentTool >= 0 &&
			std::ranges::contains(m_config.tools[static_cast<size_t>(m_currentTool)].heaters, static_cast<int>(heater));
		if (active && m_heaters[heater].state == "off" &&
			(inCurrentTool || m_config.heaters[heater].role != HeaterRole::tool))
		{
			m_heaters[heater].state = "active";
		}
	}

	void Machine::selectTool(int32_t tool)
	{
		if (tool == m_currentTool)
		{
			return;
		}
		if (m_currentTool >= 0)
		{
			m_tools[static_cast<size_t>(m_currentTool)].state = "standby";
			for (const int heater : m_config.tools[static_cast<size_t>(m_currentTool)].heaters)
			{
				if (m_heaters[static_cast<size_t>(heater)].state == "active")
				{
					m_heaters[static_cast<size_t>(heater)].state = "standby";
				}
			}
		}
		m_previousTool = m_currentTool;
		m_currentTool = tool;
		if (tool >= 0)
		{
			m_tools[static_cast<size_t>(tool)].state = "active";
			for (const int heater : m_config.tools[static_cast<size_t>(tool)].heaters)
			{
				if (m_heaters[static_cast<size_t>(heater)].state != "fault")
				{
					m_heaters[static_cast<size_t>(heater)].state = "active";
				}
			}
		}
		if (!m_transition)
		{
			m_transition = Transition{.status = "changingTool", .remaining = TRANSITION_TIME / 2.0, .then = ""};
		}
	}

	void Machine::startJob(std::string_view path, std::string& reply)
	{
		const JobFile* file = findFile(path);
		if (file == nullptr)
		{
			reply += fmt::format("Error: can't find file {:s}\n", path);
			return;
		}
		if (m_job.running)
		{
			reply += "Error: a job is already printing\n";
			return;
		}
		m_job.running = true;
		m_job.file = static_cast<size_t>(file - m_config.files.data());
		m_job.duration = m_job.warmUp = m_job.pauseDuration = m_job.progress = 0.0;
		m_job.cancelled.assign(3, false);
		m_transition.reset();
		// As a job's start G-code does: home, then for a printer select the first tool and heat it and the bed
		for (AxisState& axis : m_axes)
		{
			if (!axis.homed)
			{
				axis.position = axis.target = m_config.axes[static_cast<size_t>(&axis - m_axes.data())].min;
				axis.homed = true;
			}
		}
		m_job.warming = false;
		if (m_config.mode == MachineMode::fff)
		{
			if (m_currentTool < 0 && !m_tools.empty())
			{
				selectTool(0);
				m_transition.reset();
			}
			std::string ignored;
			for (size_t h = 0; h < m_config.heaters.size(); h++)
			{
				const Heater& heater = m_config.heaters[h];
				const bool toolHeater =
					m_currentTool >= 0 &&
					std::ranges::contains(m_config.tools[static_cast<size_t>(m_currentTool)].heaters,
										  static_cast<int>(h));
				if (heater.role == HeaterRole::bed || toolHeater)
				{
					const float target = heater.role == HeaterRole::bed ? 60.0f : 210.0f;
					setHeaterTemperature(h, std::min(target, heater.max), std::nullopt, ignored);
					m_heaters[h].state = m_heaters[h].state == "fault" ? "fault" : "active";
				}
			}
			m_job.warming = true;
		}
	}

	const JobFile* Machine::findFile(std::string_view path) const
	{
		const size_t slash = path.find_last_of('/');
		const std::string_view name = slash == std::string_view::npos ? path : path.substr(slash + 1);
		for (const JobFile& file : m_config.files)
		{
			if (file.name == name)
			{
				return &file;
			}
		}
		return nullptr;
	}
} // namespace Sim

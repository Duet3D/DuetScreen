/*
 * SerialLink.cpp
 */

#include "SerialLink.h"
#include "Comm/Gcode.h"
#include "Comm/Lines.h"
#include "Debug.h"
#include "Duet3D/General/CRC16.h"
#include "ObjectModel/Tree/Json.h"
#include "tracy/Tracy.hpp"
#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <ranges>
#include <vector>

namespace Comm
{
	namespace
	{
		/** Commands that go on the status channel when there are two: state requests, and what must not wait */
		constexpr std::array<std::string_view, 7> statusChannelCommands = {
			"M409",
			"M112",
			"M999",
			"M111",
			"M122",
			"M108",
			"M25",
		};

		/** A request whose small reply says every line sent before it has been dealt with */
		constexpr std::string_view acknowledgement = R"(M409 K"state.status" F"v")";

		/** Why a request still waiting when the link closed has no reply */
		ReplyError closedError()
		{
			return ReplyError{.message = "the connection closed"};
		}
	} // namespace

	SerialLink::SerialLink(SerialPort& port, UnaskedHandler unasked)
		: m_port(port)
		, m_unasked(std::move(unasked))
		, m_lastHeard(Clock::now())
	{
	}

	void SerialLink::receive(size_t channel, std::string_view line)
	{
		ZoneScoped;
		ZoneValue(line.size());
		m_lastHeard = Clock::now();
		auto reply = OM::parseJson(line);
		if (!reply)
		{
			LOG_WARN("Channel {:d} received a line that isn't JSON ({:s} at {:d}): {:s}",
					 channel,
					 reply.error().message,
					 reply.error().position,
					 line);
			return;
		}
		const OM::Node& number = (*reply)["line"];
		if (number.kind() != OM::Node::Kind::number)
		{
			m_unasked(*reply);
			return;
		}
		const auto lineNumber = static_cast<uint32_t>(number.integer());
		if (const auto it = m_pending.find(lineNumber); it != m_pending.end())
		{
			ReplyHandler handler = std::move(it->second.handler);
			m_pending.erase(it);
			TracyPlot("Serial requests waiting", static_cast<int64_t>(m_pending.size()));
			handler(std::move(*reply));
			return;
		}
		// An error from a line of a file job fails the job; it is shown too, as any response is
		if (const std::string_view response = (*reply)["resp"].text(); response.starts_with("Error"))
		{
			for (auto& [id, job] : m_jobs)
			{
				if (lineNumber >= job.first && lineNumber <= job.last && !job.error)
				{
					job.error = ReplyError{.message = std::string(response)};
				}
			}
		}
		m_unasked(*reply);
	}

	void SerialLink::requestModel(std::string_view key, std::string_view flags, ModelHandler handler)
	{
		ZoneScoped;
		if (key.empty())
		{
			request(fmt::format("M409 F{:s}", Comm::quoted(flags)), std::move(handler));
		}
		else
		{
			request(fmt::format("M409 K{:s} F{:s}", Comm::quoted(key), Comm::quoted(flags)), std::move(handler));
		}
	}

	void SerialLink::sendGcode(std::string_view gcode)
	{
		ZoneScoped;
		forEachLine(gcode, EmptyLines::skip, [this](std::string_view line) { sendLine(line); });
	}

	void SerialLink::emergencyStop()
	{
		ZoneScoped;
		sendLine("M112 M999");
	}

	void SerialLink::listFiles(std::string_view dir, size_t first, Handler<FileList> handler)
	{
		ZoneScoped;
		request(fmt::format("M20 S3 P{:s} R{:d}", Comm::quoted(dir), first),
				[handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
				{ handler(reply.and_then([](const OM::Node& json) { return FileList::read(json); })); });
	}

	void SerialLink::fileInfo(std::string_view file, Handler<FileInfo> handler)
	{
		ZoneScoped;
		request(fmt::format("M36 {:s}", Comm::quoted(file)),
				[handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
				{ handler(reply.and_then([](const OM::Node& json) { return FileInfo::read(json); })); });
	}

	void SerialLink::thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler)
	{
		ZoneScoped;
		request(fmt::format("M36.1 P{:s} S{:d}", Comm::quoted(file), offset),
				[handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
				{ handler(reply.and_then([](const OM::Node& json) { return FileChunk::read(json, "thumbnail"); })); });
	}

	void SerialLink::readFile(std::string_view file, Handler<std::string> handler)
	{
		ZoneScoped;
		readFrom(std::string(file), 0, std::string(), std::move(handler));
	}

	void SerialLink::readFrom(const std::string& file,
							  uint32_t offset,
							  std::string contents,
							  Handler<std::string> handler)
	{
		ZoneScoped;
		request(fmt::format("M36.2 P{:s} S{:d}", Comm::quoted(file), offset),
				[this, file, contents = std::move(contents), handler = std::move(handler)](
					std::expected<OM::Node, ReplyError> reply) mutable
				{
					ZoneScopedN("readFrom_cb");
					const auto chunk =
						reply.and_then([](const OM::Node& json) { return FileChunk::read(json, "fragment"); });
					if (!chunk)
					{
						handler(std::unexpected(chunk.error()));
						return;
					}
					contents += chunk->data;
					if (chunk->next == 0)
					{
						handler(std::move(contents));
						return;
					}
					readFrom(file, chunk->next, std::move(contents), std::move(handler));
				});
	}

	void SerialLink::writeFile(std::string_view file, std::string contents, Handler<void> handler)
	{
		ZoneScoped;
		if (contents.size() > m_port.maxUploadSize())
		{
			handler(std::unexpected(
				ReplyError{.message = fmt::format("{:d} bytes is too large to send this way (the limit is {:d})",
												  contents.size(),
												  m_port.maxUploadSize())}));
			return;
		}
		// A line at a time: `echo >` starts the file, `echo >>` adds to it
		std::vector<std::string> commands;
		forEachLine(
			contents,
			EmptyLines::keep,
			[&](std::string_view line)
			{
				commands.push_back(fmt::format(
					"echo {:s}{:s} {:s}", commands.empty() ? ">" : ">>", Comm::quoted(file), Comm::quoted(line)));
			});
		runFileJob(commands, std::move(handler));
	}

	void SerialLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		ZoneScoped;
		runFileJob({fmt::format("M30 {:s}", Comm::quoted(file))}, std::move(handler));
	}

	void SerialLink::runFileJob(const std::vector<std::string>& commands, Handler<void> handler)
	{
		ZoneScoped;
		if (m_closed)
		{
			handler(std::unexpected(closedError()));
			return;
		}
		// Every line must fit before any is sent, so a file is never left half written for that
		for (size_t i = 0; i < commands.size(); i++)
		{
			if (frame(m_nextLine + static_cast<uint32_t>(i), commands[i]).size() > maxLineLength)
			{
				handler(std::unexpected(ReplyError{
					.message = fmt::format(
						"line {:d} is longer than RepRapFirmware takes ({:d} characters)", i + 1, maxLineLength)}));
				return;
			}
		}
		// The acknowledgement goes on the job's channel: only there does its reply follow the job's lines
		const size_t channel = commands.empty() ? 0 : channelFor(commands.front());
		const uint64_t id = m_nextJob++;
		FileJob job{.first = m_nextLine, .last = m_nextLine, .error = std::nullopt};
		for (const std::string& command : commands)
		{
			sendLine(command, channel);
		}
		job.last = m_nextLine - 1;
		m_jobs.emplace(id, job);
		request(
			acknowledgement,
			[this, id, handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
			{
				ZoneScopedN("runFileJob_cb");
				const auto it = m_jobs.find(id);
				const std::optional<ReplyError> error = it != m_jobs.end() ? it->second.error : std::nullopt;
				if (it != m_jobs.end())
				{
					m_jobs.erase(it);
				}
				if (!reply)
				{
					handler(std::unexpected(reply.error()));
				}
				else if (error)
				{
					handler(std::unexpected(*error));
				}
				else
				{
					handler({});
				}
			},
			channel);
	}

	void SerialLink::expire(Clock::time_point now)
	{
		ZoneScoped;
		std::vector<Pending> expired;
		for (auto it = m_pending.begin(); it != m_pending.end();)
		{
			if (it->second.due <= now)
			{
				expired.push_back(std::move(it->second));
				it = m_pending.erase(it);
			}
			else
			{
				++it;
			}
		}
		if (expired.empty())
		{
			return;
		}
		TracyPlot("Serial requests waiting", static_cast<int64_t>(m_pending.size()));
		for (Pending& request : expired)
		{
			LOG_WARN("No reply to \"{:s}\"", request.command);
			request.handler(std::unexpected(ReplyError{.message = fmt::format("no reply to {:s}", request.command)}));
		}
	}

	std::optional<SerialLink::Clock::time_point> SerialLink::nextDue() const
	{
		if (m_pending.empty())
		{
			return std::nullopt;
		}
		return std::ranges::min(m_pending | std::views::values | std::views::transform(&Pending::due));
	}

	void SerialLink::close()
	{
		ZoneScoped;
		if (m_closed)
		{
			return;
		}
		m_closed = true;
		std::map<uint32_t, Pending> pending = std::move(m_pending);
		m_pending.clear();
		m_jobs.clear();
		TracyPlot("Serial requests waiting", static_cast<int64_t>(0));
		for (auto& [line, request] : pending)
		{
			request.handler(std::unexpected(closedError()));
		}
	}

	void SerialLink::request(std::string_view command, ReplyHandler handler, std::optional<size_t> channel)
	{
		ZoneScoped;
		ZoneText(command.data(), command.size());
		const std::optional<uint32_t> line = sendLine(command, channel);
		if (!line)
		{
			handler(std::unexpected(m_closed ? closedError()
											 : ReplyError{.message = fmt::format("couldn't send {:s}", command)}));
			return;
		}
		m_pending.emplace(*line,
						  Pending{.command = std::string(command),
								  .due = Clock::now() + m_port.replyTimeout(),
								  .handler = std::move(handler)});
		TracyPlot("Serial requests waiting", static_cast<int64_t>(m_pending.size()));
	}

	std::string SerialLink::frame(uint32_t number, std::string_view command) const
	{
		std::string line = fmt::format("N{:d} {:s}", number, command);
		if (m_port.needsChecksums())
		{
			CRC16 crc;
			crc.Update(line.data(), line.size());
			line += fmt::format("*{:05d}", crc.Get());
		}
		return line;
	}

	std::optional<uint32_t> SerialLink::sendLine(std::string_view command, std::optional<size_t> channel)
	{
		ZoneScoped;
		ZoneText(command.data(), command.size());
		if (m_closed)
		{
			return std::nullopt;
		}
		const uint32_t number = m_nextLine;
		std::string line = frame(number, command);
		if (line.size() > maxLineLength)
		{
			LOG_ERROR("Not sending \"{:s}\": longer than RepRapFirmware takes", command);
			return std::nullopt;
		}
		m_nextLine++;
		LOG_DBG("Sending: {:s}", line);
		line += '\n';
		// One write per line: a line sent in parts could be left half sent, and the next joined onto it
		if (!m_port.write(channel.value_or(channelFor(command)), line))
		{
			LOG_WARN("Couldn't send \"{:s}\"", command);
			m_failed = true;
		}
		return number;
	}

	size_t SerialLink::channelFor(std::string_view command) const
	{
		if (m_port.channels() < 2)
		{
			return 0;
		}
		const size_t start = command.find_first_not_of(" \t");
		const std::string_view word = start == std::string_view::npos
										  ? std::string_view()
										  : command.substr(start, command.find_first_of(" \t", start) - start);
		return std::ranges::find(statusChannelCommands, word) != statusChannelCommands.end() ? 1 : 0;
	}
} // namespace Comm

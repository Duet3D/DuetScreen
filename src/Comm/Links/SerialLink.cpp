/*
 * SerialLink.cpp
 */

#include "SerialLink.h"
#include "Comm/Gcode.h"
#include "Debug.h"
#include "Duet3D/General/CRC16.h"
#include "ObjectModel/Tree/Json.h"
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

		/** Why a request still waiting when the link closed has no reply */
		ReplyError closedError()
		{
			return ReplyError{.message = "the connection closed"};
		}
	} // namespace

	SerialLink::SerialLink(SerialPort& port, UnaskedHandler unasked, Options options)
		: m_port(port)
		, m_unasked(std::move(unasked))
		, m_options(options)
		, m_lastHeard(Clock::now())
	{
	}

	SerialLink::~SerialLink()
	{
		std::map<uint32_t, Pending> pending = std::move(m_pending);
		m_pending.clear();
		for (auto& [line, request] : pending)
		{
			request.handler(std::unexpected(closedError()));
		}
	}

	void SerialLink::receive(size_t channel, std::string_view line)
	{
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
		if (number.kind() == OM::Node::Kind::number)
		{
			const auto it = m_pending.find(static_cast<uint32_t>(number.integer()));
			if (it != m_pending.end())
			{
				ReplyHandler handler = std::move(it->second.handler);
				m_pending.erase(it);
				handler(std::move(*reply));
				return;
			}
		}
		m_unasked(*reply);
	}

	std::optional<SerialLink::Clock::time_point> SerialLink::nextDeadline() const
	{
		if (m_pending.empty())
		{
			return std::nullopt;
		}
		return std::ranges::min(m_pending | std::views::values | std::views::transform(&Pending::due));
	}

	void SerialLink::requestModel(std::string_view key, std::string_view flags, ModelHandler handler)
	{
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
		while (!gcode.empty())
		{
			const size_t end = gcode.find('\n');
			std::string_view line = gcode.substr(0, end);
			if (!line.empty() && line.back() == '\r')
			{
				line.remove_suffix(1);
			}
			if (!line.empty()) // an empty line says nothing
			{
				sendLine(line);
			}
			gcode = end == std::string_view::npos ? std::string_view() : gcode.substr(end + 1);
		}
	}

	void SerialLink::emergencyStop()
	{
		sendLine("M112 M999");
	}

	void SerialLink::listFiles(std::string_view dir, size_t first, Handler<FileList> handler)
	{
		request(fmt::format("M20 S3 P{:s} R{:d}", Comm::quoted(dir), first),
				[handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
				{ handler(reply.and_then([](const OM::Node& json) { return FileList::read(json); })); });
	}

	void SerialLink::fileInfo(std::string_view file, Handler<FileInfoReply> handler)
	{
		request(fmt::format("M36 {:s}", Comm::quoted(file)),
				[handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
				{ handler(reply.and_then([](const OM::Node& json) { return FileInfoReply::read(json); })); });
	}

	void SerialLink::thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler)
	{
		request(fmt::format("M36.1 P{:s} S{:d}", Comm::quoted(file), offset),
				[handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
				{ handler(reply.and_then([](const OM::Node& json) { return FileChunk::read(json, "thumbnail"); })); });
	}

	void SerialLink::readFile(std::string_view file, Handler<std::string> handler)
	{
		readFrom(std::string(file), 0, std::string(), std::move(handler));
	}

	void SerialLink::readFrom(const std::string& file,
							  uint32_t offset,
							  std::string contents,
							  Handler<std::string> handler)
	{
		request(fmt::format("M36.2 P{:s} S{:d}", Comm::quoted(file), offset),
				[this, file, contents = std::move(contents), handler = std::move(handler)](
					std::expected<OM::Node, ReplyError> reply) mutable
				{
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
		if (contents.size() > m_options.maxUploadSize)
		{
			handler(std::unexpected(
				ReplyError{.message = fmt::format("{:d} bytes is too large to send this way (the limit is {:d})",
												  contents.size(),
												  m_options.maxUploadSize)}));
			return;
		}
		// A line at a time: `echo >` starts the file, `echo >>` adds to it. (M559 would be quicker once DSF has it.)
		std::string_view rest = contents;
		bool first = true;
		do
		{
			const size_t end = rest.find('\n');
			sendLine(fmt::format(
				"echo {:s}{:s} {:s}", first ? ">" : ">>", Comm::quoted(file), Comm::quoted(rest.substr(0, end))));
			first = false;
			rest = end == std::string_view::npos ? std::string_view() : rest.substr(end + 1);
		} while (!rest.empty());
		handler({});
	}

	void SerialLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		sendLine(fmt::format("M30 {:s}", Comm::quoted(file)));
		handler({});
	}

	void SerialLink::expire(Clock::time_point now)
	{
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
		for (Pending& request : expired)
		{
			LOG_WARN("No reply to \"{:s}\"", request.command);
			request.handler(std::unexpected(ReplyError{.message = fmt::format("no reply to {:s}", request.command)}));
		}
	}

	void SerialLink::request(std::string_view command, ReplyHandler handler)
	{
		const uint32_t line = sendLine(command);
		m_pending.emplace(line,
						  Pending{.command = std::string(command),
								  .due = Clock::now() + m_options.replyTimeout,
								  .handler = std::move(handler)});
	}

	uint32_t SerialLink::sendLine(std::string_view command)
	{
		const uint32_t number = m_nextLine++;
		std::string line = fmt::format("N{:d} {:s}", number, command);
		if (m_port.needsChecksums())
		{
			CRC16 crc;
			crc.Update(line.data(), line.size());
			line += fmt::format("*{:05d}", crc.Get());
		}
		line += '\n';
		LOG_DBG("Sending: {:s}", std::string_view(line).substr(0, line.size() - 1));
		// One write per line: a line sent in parts could be left half sent, and the next joined onto it
		if (!m_port.write(channelFor(command), line))
		{
			LOG_WARN("Couldn't send \"{:s}\"", command);
			m_failed = true;
		}
		return number;
	}

	size_t SerialLink::channelFor(std::string_view command) const
	{
		if (m_port.channels() < 2 || !m_options.useStatusChannel)
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

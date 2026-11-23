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

		/** What ends a file sent with M559 without a size: how an empty file is written (S0 means "no size") */
		constexpr std::string_view endOfFile = "<!-- **EoF** -->";

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
		if (holding())
		{
			hold([this, gcode = std::string(gcode)] { sendGcode(gcode); });
			return;
		}
		forEachLine(gcode, EmptyLines::skip, [this](std::string_view line) { sendLine(line); });
	}

	void SerialLink::emergencyStop()
	{
		ZoneScoped;
		constexpr std::string_view command = "M112 M999";
		// Once M559 is sent, the printer may take whatever follows on its channel as part of the file
		if (holding() && m_upload->stage != Upload::Stage::waitingForChannel && channelFor(command) == fileChannel)
		{
			LOG_WARN("The emergency stop is sent once the file being written has been");
			m_held.push_front([this] { emergencyStop(); });
			return;
		}
		sendLine(command);
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

	void SerialLink::writeFile(std::string_view file, FileContents contents, Progress progress, Handler<void> handler)
	{
		ZoneScoped;
		if (holding())
		{
			hold([this,
				  file = std::string(file),
				  contents = std::move(contents),
				  progress = std::move(progress),
				  handler = std::move(handler)]() mutable
				 { writeFile(file, std::move(contents), std::move(progress), std::move(handler)); });
			return;
		}
		if (m_closed)
		{
			handler(std::unexpected(closedError()));
			return;
		}
		// Checked with the longest line number: an emergency stop may go ahead of M559 while the channel is waited for
		if (frame(UINT32_MAX, fmt::format("M559 P{:s} S{:d} C{:d}", Comm::quoted(file), contents.size(), UINT32_MAX))
				.size() > maxLineLength)
		{
			handler(std::unexpected(ReplyError{
				.message = fmt::format("the file's name is too long to send (lines are at most {:d} characters)",
									   maxLineLength)}));
			return;
		}
		const size_t size = contents.size();
		m_upload = Upload{.stage = Upload::Stage::waitingForChannel,
						  .file = std::string(file),
						  .bytes = size == 0 ? FileContents::bytes(std::string(endOfFile)) : std::move(contents),
						  .size = size,
						  .written = 0,
						  .line = 0,
						  .unreadable = std::nullopt,
						  .progress = std::move(progress),
						  .handler = std::move(handler)};
		requestNow(
			acknowledgement, [this](std::expected<OM::Node, ReplyError> reply) { channelFree(reply); }, fileChannel);
	}

	void SerialLink::channelFree(const std::expected<OM::Node, ReplyError>& reply)
	{
		ZoneScoped;
		if (!m_upload)
		{
			return;
		}
		if (!reply)
		{
			uploadFailed(m_closed
							 ? closedError()
							 : ReplyError{.message = fmt::format("the printer is busy ({:s})", reply.error().message)});
			return;
		}
		Upload& upload = *m_upload;
		if (upload.bytes.cancelled())
		{
			uploadFailed(FileContents::cancelledError()); // nothing sent yet, so nothing to finish
			return;
		}
		upload.stage = Upload::Stage::opening;
		std::string command;
		if (upload.size == 0)
		{
			command = fmt::format("M559 P{:s}", Comm::quoted(upload.file));
		}
		else
		{
			command =
				fmt::format("M559 P{:s} S{:d} C{:d}", Comm::quoted(upload.file), upload.size, upload.bytes.crc32());
		}
		upload.line = m_nextLine;
		requestNow(command, [this](std::expected<OM::Node, ReplyError> reply) { uploadOpened(reply); }, fileChannel);
	}

	void SerialLink::uploadOpened(const std::expected<OM::Node, ReplyError>& reply)
	{
		ZoneScoped;
		if (!m_upload)
		{
			return;
		}
		if (!reply)
		{
			if (!m_closed)
			{
				// The printer may take M559 yet, and the next bytes sent with it: only connecting again is safe
				LOG_ERROR("No reply to M559: connecting again");
				m_failed = true;
			}
			uploadFailed(m_closed ? closedError() : reply.error());
			return;
		}
		if (const std::string_view response = (*reply)["resp"].text(); response.starts_with("Error"))
		{
			uploadFailed(ReplyError{.message = std::string(response)});
			return;
		}
		m_upload->stage = Upload::Stage::sending; // the next tick() sends the first piece
	}

	void SerialLink::sendPiece()
	{
		ZoneScoped;
		Upload& upload = *m_upload;
		std::expected<std::string, ReplyError> piece = std::unexpected(ReplyError{});
		if (!upload.unreadable)
		{
			piece = upload.bytes.read(upload.written, filePieceSize);
			if (!piece)
			{
				// The printer takes the next bytes as the file, whatever they are: filler keeps it in step, and its
				// CRC won't match, so the printer refuses the file
				LOG_WARN("Couldn't read the next piece of {:s} ({:s}): sending the rest as filler",
						 upload.file,
						 piece.error().message);
				upload.unreadable = piece.error();
			}
		}
		if (upload.unreadable)
		{
			piece = std::string(std::min(filePieceSize, upload.bytes.size() - upload.written), ' ');
		}
		const size_t length = piece->size();
		ZoneValue(length);
		if (!m_port.write(fileChannel, *piece))
		{
			LOG_WARN("Couldn't send a piece of {:s}", upload.file);
			m_failed = true;
			uploadFailed(ReplyError{.message = "the file couldn't be sent"});
			return;
		}
		upload.written += length;
		// Nothing else is sent while the file is, so nothing else is heard: its being taken is what is heard
		m_lastHeard = Clock::now();
		if (upload.progress)
		{
			upload.progress(std::min(upload.written, upload.size), upload.size);
		}
		if (upload.written < upload.bytes.size())
		{
			return;
		}
		Upload sent = std::move(*m_upload);
		m_upload.reset();
		Handler<void> handler = std::move(sent.handler);
		if (sent.unreadable)
		{
			// The printer refuses it; why is that it couldn't be read
			handler = [handler = std::move(handler), why = *sent.unreadable](std::expected<void, ReplyError>)
			{ handler(std::unexpected(why)); };
		}
		acknowledge(sent.line, sent.line, fileChannel, std::move(handler));
		releaseHeld();
	}

	void SerialLink::uploadFailed(ReplyError error)
	{
		ZoneScoped;
		LOG_WARN("Writing {:s} failed: {:s}", m_upload->file, error.message);
		Upload failed = std::move(*m_upload);
		m_upload.reset();
		failed.handler(std::unexpected(std::move(error)));
		releaseHeld();
	}

	void SerialLink::hold(std::function<void()> task)
	{
		m_held.push_back(std::move(task));
		TracyPlot("Serial requests held", static_cast<int64_t>(m_held.size()));
	}

	void SerialLink::releaseHeld()
	{
		ZoneScoped;
		while (!holding() && !m_held.empty())
		{
			std::function<void()> task = std::move(m_held.front());
			m_held.pop_front();
			task();
		}
		TracyPlot("Serial requests held", static_cast<int64_t>(m_held.size()));
	}

	void SerialLink::deleteFile(std::string_view file, Handler<void> handler)
	{
		ZoneScoped;
		if (holding())
		{
			hold([this, file = std::string(file), handler = std::move(handler)]() mutable
				 { deleteFile(file, std::move(handler)); });
			return;
		}
		const std::string command = fmt::format("M30 {:s}", Comm::quoted(file));
		const std::optional<uint32_t> line = sendLine(command);
		if (!line)
		{
			handler(std::unexpected(m_closed ? closedError()
											 : ReplyError{.message = fmt::format("couldn't send {:s}", command)}));
			return;
		}
		// The acknowledgement goes on the command's channel: only there does its reply follow the command
		acknowledge(*line, *line, channelFor(command), std::move(handler));
	}

	void SerialLink::acknowledge(uint32_t first, uint32_t last, size_t channel, Handler<void> handler)
	{
		ZoneScoped;
		const uint64_t id = m_nextJob++;
		m_jobs.emplace(id, FileJob{.first = first, .last = last, .error = std::nullopt});
		requestNow(
			acknowledgement,
			[this, id, handler = std::move(handler)](std::expected<OM::Node, ReplyError> reply)
			{
				ZoneScopedN("acknowledge_cb");
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

	void SerialLink::tick(Clock::time_point now)
	{
		ZoneScoped;
		if (m_upload && m_upload->stage == Upload::Stage::sending)
		{
			sendPiece();
		}
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
		if (m_upload && m_upload->stage == Upload::Stage::sending)
		{
			return Clock::now(); // the next piece
		}
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
		// A file being written, and what was held back for it, are told too
		if (m_upload)
		{
			uploadFailed(closedError());
		}
		releaseHeld();
	}

	void SerialLink::request(std::string_view command, ReplyHandler handler, std::optional<size_t> channel)
	{
		ZoneScoped;
		if (holding())
		{
			hold([this, command = std::string(command), handler = std::move(handler), channel]() mutable
				 { request(command, std::move(handler), channel); });
			return;
		}
		requestNow(command, std::move(handler), channel);
	}

	void SerialLink::requestNow(std::string_view command, ReplyHandler handler, std::optional<size_t> channel)
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

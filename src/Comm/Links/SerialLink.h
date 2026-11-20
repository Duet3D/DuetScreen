/*
 * SerialLink.h
 *
 * A printer reached by G-code lines over a SerialPort (USB, UART, the simulated Duet), as PanelDue reaches it: every
 * line is numbered (`N12 M409 K"heat"`), with a checksum where the port needs one, and each JSON reply comes back as
 * a line.
 *
 * Replies are matched to requests by line number: RepRapFirmware (3.6.3 on) puts the number of the line a reply
 * answers in it ("line":12), and leaves it out of what it sends by itself (status reports while a channel is busy).
 * So a reply is always given to the request it answers, however late or out of order; a reply that answers nothing
 * waiting goes to the unasked handler (status reports, responses to G-code, replies that came after their request
 * was given up).
 *
 * Writing and deleting a file are G-code with no reply of their own (echo, M30): an error names its line, and a final
 * request's reply says every line before it has been dealt with, so the job is only reported done once it is.
 *
 * With two channels (a Duet 3 over USB), requests for the printer's state and emergency commands go on the second,
 * so they are answered while the first is busy (heating, a long move, a macro).
 */

#pragma once

#include "Comm/Link.h"
#include "Comm/Ports/SerialPort.h"
#include "ObjectModel/Tree/Node.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace Comm
{
	class SerialLink : public Link
	{
	  public:
		using Clock = std::chrono::steady_clock; // What deadlines are measured with

		/** Receives a reply that answers nothing waiting for one */
		using UnaskedHandler = std::function<void(OM::Node& reply)>;

		/** The longest line RepRapFirmware takes, its number and checksum included */
		static constexpr size_t maxLineLength = 255;

		/**
		 * @param port Where lines go and replies come from; open, and kept open while the link is used. What talking
		 * over it takes (reply timeout, channels, largest upload) comes from it
		 * @param unasked Given each reply that answers nothing waiting
		 */
		SerialLink(SerialPort& port, UnaskedHandler unasked);

		SerialLink(const SerialLink&) = delete;
		SerialLink& operator=(const SerialLink&) = delete;

		/** Take a whole line received on `channel` (see LineFramer): a reply */
		void receive(size_t channel, std::string_view line);

		// Link

		void requestModel(std::string_view key, std::string_view flags, ModelHandler handler) override;
		void sendGcode(std::string_view gcode) override;
		void emergencyStop() override;
		void listFiles(std::string_view dir, size_t first, Handler<FileList> handler) override;
		void fileInfo(std::string_view file, Handler<FileInfoReply> handler) override;
		void thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler) override;
		void readFile(std::string_view file, Handler<std::string> handler) override;
		void writeFile(std::string_view file, std::string contents, Handler<void> handler) override;
		void deleteFile(std::string_view file, Handler<void> handler) override;
		void expire(Clock::time_point now) override;
		std::optional<Clock::time_point> nextDue() const override;
		void close() override;
		Clock::time_point lastHeard() const override { return m_lastHeard; }
		bool failed() const override { return m_failed || m_port.failed(); }
		bool waiting() const override { return !m_pending.empty(); }

	  private:
		/** Receives a reply's JSON, or why there is none */
		using ReplyHandler = std::function<void(std::expected<OM::Node, ReplyError>)>;

		/** A request sent, waiting for its reply */
		struct Pending
		{
			std::string command;   // What was sent, for the log
			Clock::time_point due; // When it is given up
			ReplyHandler handler;  // Given the reply
		};

		/** G-code sent for a file (writing, deleting), whose lines report errors but nothing else */
		struct FileJob
		{
			uint32_t first = 0;				 // Its first line's number
			uint32_t last = 0;				 // Its last line's number
			std::optional<ReplyError> error; // The first error a line reported
		};

		/**
		 * Send `command` as a line, and give its reply to `handler`
		 * @param channel The channel it goes on; by default the one channelFor() picks
		 */
		void request(std::string_view command, ReplyHandler handler, std::optional<size_t> channel = std::nullopt);

		/**
		 * Send `command` as one numbered line
		 * @param channel The channel it goes on; by default the one channelFor() picks
		 * @return its line number, or nothing if it wasn't sent (too long, the link closed)
		 */
		std::optional<uint32_t> sendLine(std::string_view command, std::optional<size_t> channel = std::nullopt);

		/** The line `command` makes with `number`: numbered, and checksummed if the port needs it */
		std::string frame(uint32_t number, std::string_view command) const;

		/** The channel `command` goes on */
		size_t channelFor(std::string_view command) const;

		/**
		 * Send `commands` for a file, then ask for something small: its reply says every command has been dealt with,
		 * and `handler` is told whether any reported an error
		 */
		void runFileJob(const std::vector<std::string>& commands, Handler<void> handler);

		/** Ask for the piece of `file` from `offset` on (M36.2), adding it to `contents` until the file is whole */
		void readFrom(const std::string& file, uint32_t offset, std::string contents, Handler<std::string> handler);

		SerialPort& m_port;					   // Where lines go
		UnaskedHandler m_unasked;			   // Given replies nothing waits for
		uint32_t m_nextLine = 0;			   // The number the next line sent gets
		std::map<uint32_t, Pending> m_pending; // Requests waiting, by line number
		std::map<uint64_t, FileJob> m_jobs;	   // File jobs whose lines may still report errors, by id
		uint64_t m_nextJob = 1;				   // The id the next file job gets
		Clock::time_point m_lastHeard;		   // When a line was last received
		bool m_failed = false;				   // A line couldn't be written: the port has gone
		bool m_closed = false;				   // close() was called: nothing more is sent
	};
} // namespace Comm

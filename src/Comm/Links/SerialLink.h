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
 * Deleting a file is G-code with no reply of its own (M30): an error names its line, and a final request's reply says
 * every line before it has been dealt with, so the job is only reported done once it is.
 *
 * A file is written with M559, as its bytes rather than a line at a time:
 *
 *   1. a small request on the main channel, whose reply says the channel has finished what it was doing (a printer
 *      busy with an earlier command would only take the file later, and take what was sent meanwhile as part of it);
 *   2. `M559 P"<file>" S<size> C<CRC-32>`, whose reply ("Writing to file") says the printer is waiting for the bytes;
 *   3. the bytes, a piece each tick() (so the connection's thread carries on between pieces), progress told after each;
 *   4. a final request, as for M30: the printer reports a wrong CRC as an error naming M559's line.
 *
 * From the first request until the last byte is written, nothing else is sent: whatever is asked meanwhile is held,
 * and sent in the order asked once the bytes are. The printer takes the next <size> bytes on that channel as the file,
 * whatever they are, and nothing can stop it once it has started. An emergency stop goes at once on a second channel
 * if there is one; otherwise it goes first once the file's bytes are sent (or at once, before M559 is sent).
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
#include <deque>
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

		/** How many bytes of a file are written to the port each tick() */
		static constexpr size_t filePieceSize = 1024;

		/** The channel files are written on: the main one, which RepRapFirmware lets write files */
		static constexpr size_t fileChannel = 0;

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
		void fileInfo(std::string_view file, Handler<FileInfo> handler) override;
		void thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler) override;
		void readFile(std::string_view file, Handler<std::string> handler) override;
		void writeFile(std::string_view file, std::string contents, Progress progress, Handler<void> handler) override;
		void deleteFile(std::string_view file, Handler<void> handler) override;
		void tick(Clock::time_point now) override;
		std::optional<Clock::time_point> nextDue() const override;
		void close() override;
		Clock::time_point lastHeard() const override { return m_lastHeard; }
		bool failed() const override { return m_failed || m_port.failed(); }
		bool waiting() const override { return !m_pending.empty() || m_upload.has_value() || !m_held.empty(); }

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

		/** A file being written with M559 (see the file comment) */
		struct Upload
		{
			/** How far it has got */
			enum class Stage
			{
				waitingForChannel, // Waiting for the main channel to finish what it was doing
				opening,		   // M559 sent, waiting for the printer to say it waits for the bytes
				sending,		   // Its bytes being written, a piece each tick()
			};

			Stage stage = Stage::waitingForChannel; // How far it has got
			std::string file;						// Where it is written
			std::string bytes;	   // What is sent after M559: the file, or for an empty file the end-of-file marker
			size_t size = 0;	   // The file's size
			size_t written = 0;	   // How much of `bytes` has been written
			uint32_t line = 0;	   // M559's line number, which an error the printer finds with the file names
			Progress progress;	   // Told as pieces are written
			Handler<void> handler; // Told once the printer has dealt with the file, or why it can't
		};

		/** Whether a file being written holds everything else back */
		bool holding() const { return m_upload.has_value(); }

		/** Hold `task` back until the file being written has been sent, after what was held before it */
		void hold(std::function<void()> task);

		/** Do what was held back while a file was written, in order, until done or another file is being written */
		void releaseHeld();

		/**
		 * Send `command` as a line, and give its reply to `handler`; held back while a file is written
		 * @param channel The channel it goes on; by default the one channelFor() picks
		 */
		void request(std::string_view command, ReplyHandler handler, std::optional<size_t> channel = std::nullopt);

		/** request(), sent now even while a file is written (a file's own requests) */
		void requestNow(std::string_view command, ReplyHandler handler, std::optional<size_t> channel);

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
		 * Ask for something small on `channel`: its reply says every line sent before it has been dealt with, and
		 * `handler` is told whether any of the lines `first` to `last` reported an error
		 */
		void acknowledge(uint32_t first, uint32_t last, size_t channel, Handler<void> handler);

		/** The main channel has finished what it was doing (or not): ask the printer to take the file being written */
		void channelFree(const std::expected<OM::Node, ReplyError>& reply);

		/** M559 was answered (or not): send the file's bytes, or give up */
		void uploadOpened(const std::expected<OM::Node, ReplyError>& reply);

		/** Write the next piece of the file being written; after the last, wait for the printer to deal with it */
		void sendPiece();

		/** The file being written can't be, for `error`: tell its handler, and send what was held back */
		void uploadFailed(ReplyError error);

		/** Ask for the piece of `file` from `offset` on (M36.2), adding it to `contents` until the file is whole */
		void readFrom(const std::string& file, uint32_t offset, std::string contents, Handler<std::string> handler);

		SerialPort& m_port;					   // Where lines go
		UnaskedHandler m_unasked;			   // Given replies nothing waits for
		uint32_t m_nextLine = 0;			   // The number the next line sent gets
		std::map<uint32_t, Pending> m_pending; // Requests waiting, by line number
		std::map<uint64_t, FileJob> m_jobs;	   // File jobs whose lines may still report errors, by id
		uint64_t m_nextJob = 1;				   // The id the next file job gets
		std::optional<Upload> m_upload;		   // The file being written with M559, if any
		std::deque<std::function<void()>> m_held; // What was asked while a file is written, oldest first
		Clock::time_point m_lastHeard;		   // When a line was last received
		bool m_failed = false;				   // A line couldn't be written: the port has gone
		bool m_closed = false;				   // close() was called: nothing more is sent
	};
} // namespace Comm

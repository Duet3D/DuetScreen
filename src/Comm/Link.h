/*
 * Link.h
 *
 * What the screen can ask of a printer, the same however it is connected: SerialLink (USB, UART, the simulated Duet:
 * G-code lines), HttpLink (a standalone Duet's rr_* requests) and DsfLink (a Duet with an SBC). The Connection
 * (Connection.h) owns one and calls it on its own thread only; every answer comes back on that thread too, once, as
 * the value asked for or why there is none. close() answers whatever is still waiting; a link destroyed without being
 * closed (the program ending) calls nothing.
 */

#pragma once

#include "Comm/ModelSync.h"
#include "Comm/Replies.h"
#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace Comm
{
	/** Receives an answer: `T`, or why there is none */
	template <typename T>
	using Handler = std::function<void(std::expected<T, ReplyError>)>;

	/** Told how much of a file has been sent: `sent` of `total` bytes */
	using Progress = std::function<void(size_t sent, size_t total)>;

	class Link : public ModelLink
	{
	  public:
		/** Whether the link keeps the object model up to date by itself (DSF's WebSocket), so ModelSync isn't used */
		virtual bool pushesModel() const { return false; }

		/** Send G-code: one or more lines. Its responses are shown as they arrive (Comm::showResponse()). */
		virtual void sendGcode(std::string_view gcode) = 0;

		/** Stop the machine now (M112), then reset it (M999), ahead of anything else waiting */
		virtual void emergencyStop() = 0;

		/** List the folder `dir` (e.g. "0:/gcodes") from its item `first` on: one page */
		virtual void listFiles(std::string_view dir, size_t first, Handler<FileList> handler) = 0;

		/** What the firmware knows about the G-code file `file` */
		virtual void fileInfo(std::string_view file, Handler<FileInfo> handler) = 0;

		/** The piece of a thumbnail in `file` starting at `offset` */
		virtual void thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler) = 0;

		/** The whole of `file` */
		virtual void readFile(std::string_view file, Handler<std::string> handler) = 0;

		/**
		 * Write `contents` to `file`, replacing it
		 * @param progress Told as pieces of it are sent, where the link sends it in pieces (a serial link); perhaps
		 * never
		 * @param handler Told once the printer has the file, or why it hasn't
		 */
		virtual void writeFile(std::string_view file, std::string contents, Progress progress, Handler<void> handler) = 0;

		/** Delete `file` */
		virtual void deleteFile(std::string_view file, Handler<void> handler) = 0;

		/**
		 * Let time pass: do what is due by `now`. A request not answered by its deadline is given up (its handler
		 * told); a file being sent in pieces gets its next piece.
		 */
		virtual void tick(std::chrono::steady_clock::time_point now) = 0;

		/** When tick() next has something to do, if anything */
		virtual std::optional<std::chrono::steady_clock::time_point> nextDue() const = 0;

		/** Give up everything still waiting, telling each handler there is no answer, and end the session */
		virtual void close() = 0;

		/** When the printer last sent anything (or took a piece of a file being sent, when nothing else is heard) */
		virtual std::chrono::steady_clock::time_point lastHeard() const = 0;

		/** Whether the link can't go on (its port has gone, or the printer forgot the session): connect again */
		virtual bool failed() const = 0;

		/** Whether anything asked is still waiting for its answer */
		virtual bool waiting() const = 0;
	};
} // namespace Comm

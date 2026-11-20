/*
 * Link.h
 *
 * What the screen can ask of a printer, the same however it is connected: SerialLink (USB, UART, the simulated Duet:
 * G-code lines), HttpLink (a standalone Duet's rr_* requests) and DsfLink (a Duet with an SBC). The Connection
 * (Connection.h) owns one and calls it on its own thread only; every answer comes back on that thread too, once, as
 * the value asked for or why there is none.
 */

#pragma once

#include "Comm/ModelSync.h"
#include "Comm/Replies.h"
#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <string_view>

namespace Comm
{
	/** Receives an answer: `T`, or why there is none */
	template <typename T>
	using Handler = std::function<void(std::expected<T, ReplyError>)>;

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
		virtual void fileInfo(std::string_view file, Handler<FileInfoReply> handler) = 0;

		/** The piece of a thumbnail in `file` starting at `offset` */
		virtual void thumbnail(std::string_view file, uint32_t offset, Handler<FileChunk> handler) = 0;

		/** The whole of `file` */
		virtual void readFile(std::string_view file, Handler<std::string> handler) = 0;

		/** Write `contents` to `file`, replacing it */
		virtual void writeFile(std::string_view file, std::string contents, Handler<void> handler) = 0;

		/** Delete `file` */
		virtual void deleteFile(std::string_view file, Handler<void> handler) = 0;

		/** Let time pass: a request not answered by its deadline is given up (its handler told) */
		virtual void expire(std::chrono::steady_clock::time_point now) = 0;

		/** When the printer last sent anything */
		virtual std::chrono::steady_clock::time_point lastHeard() const = 0;

		/** Whether the link can't go on (its port has gone, or the printer forgot the session): connect again */
		virtual bool failed() const = 0;

		/** Whether anything asked is still waiting for its answer */
		virtual bool waiting() const = 0;
	};
} // namespace Comm

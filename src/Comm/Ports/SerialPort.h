/*
 * SerialPort.h
 *
 * Where G-code lines go out and replies come in, for SerialLink: USB (one channel, or two on a Duet 3), UART, the
 * simulated Duet, or a printer a test plays. A port only moves bytes, and says what talking over it takes; SerialLink
 * numbers the lines and matches the replies.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace Comm
{
	class SerialPort
	{
	  public:
		/** Receives the bytes a channel received, on any thread, in pieces of any size */
		using Receiver = std::function<void(size_t channel, std::string_view bytes)>;

		virtual ~SerialPort() = default;

		/**
		 * Open the port
		 * @param receiver Given everything received from now until close()
		 * @return whether it opened
		 */
		virtual bool open(Receiver receiver) = 0;

		/** Close the port; nothing more is received */
		virtual void close() = 0;

		/**
		 * Write `bytes` to `channel`
		 * @return false if they couldn't be written (e.g. the device has gone)
		 */
		virtual bool write(size_t channel, std::string_view bytes) = 0;

		/** How many channels it has: 1, or 2 for a Duet 3 over USB with its second channel used */
		virtual size_t channels() const = 0;

		/** Whether each line needs a checksum (UART, where bytes can be lost) */
		virtual bool needsChecksums() const = 0;

		/** Whether the port has gone (unplugged, or an error reading it): the connection starts again */
		virtual bool failed() const { return false; }

		/** G-code the printer needs before anything else over this port, a line each (USB: JSON replies) */
		virtual std::vector<std::string> openingGcode() const { return {}; }

		/** How long a request may wait for its reply over this port */
		virtual std::chrono::milliseconds replyTimeout() const { return std::chrono::milliseconds(5000); }
	};
} // namespace Comm

/*
 * SerialPort.h
 *
 * Where G-code lines go out and replies come in, for SerialLink: USB (one channel, or two on a Duet 3), UART, or the
 * simulated Duet. A port only moves bytes; SerialLink numbers the lines and matches the replies.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

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

		/** How many channels it has: 1, or 2 for a Duet 3 over USB */
		virtual size_t channels() const = 0;

		/** Whether each line needs a checksum (UART, where bytes can be lost) */
		virtual bool needsChecksums() const = 0;
	};
} // namespace Comm

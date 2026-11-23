/*
 * LineFramer.h
 *
 * Over USB and UART the printer sends one reply per line, but the bytes arrive in pieces of any size: a piece may end
 * in the middle of a reply, or hold several. A LineFramer gathers the pieces of one channel and hands on each whole
 * line, so a reply is only ever read whole.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace Comm
{
	class LineFramer
	{
	  public:
		/** Receives each whole line, without its line ending; never an empty line */
		using LineHandler = std::function<void(std::string_view line)>;

		/** The longest line kept: a longer one is dropped (it would be a garbled reply), up to its end */
		static constexpr size_t maxLineLength = 256 * 1024;

		/** @param handler Receives each whole line */
		explicit LineFramer(LineHandler handler);

		/** Add the bytes received next, handing on every line they complete */
		void push(std::string_view bytes);

	  private:
		/** Hand on the line gathered, if it isn't empty, and start the next */
		void endLine();

		LineHandler m_handler;	 // Receives each whole line
		std::string m_line;		 // The line being gathered
		bool m_dropping = false; // The line being received is too long and is being skipped up to its end
	};
} // namespace Comm

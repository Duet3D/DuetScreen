/*
 * FileTransfer.h
 *
 * How writing a file to the printer is going, as the UI is told it (EventType::FileTransfer): once when the write
 * starts, as its pieces are sent (over USB and UART; the network sends a file whole), and once when it ends.
 */

#pragma once

#include <cstddef>
#include <string>

namespace Comm
{
	/** How writing a file to the printer is going */
	struct FileTransfer
	{
		std::string file;	   // Where it is written, e.g. "0:/sys/duetscreen.json"
		size_t sent = 0;	   // Bytes sent so far
		size_t total = 0;	   // Bytes in all
		bool finished = false; // The write has ended, written or not (whoever asked for it is told which)
	};
} // namespace Comm

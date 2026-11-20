/*
 * Receive.h
 *
 * Where what the printer sends goes. Each reply is read whole as JSON (OM::parseJson()) and handed to what it is for:
 * the object model, the folder listing, file information or thumbnail asked for, the file being read, or the user (a
 * response to show).
 */

#pragma once

#include "ObjectModel/Tree/Node.h"
#include <string_view>

namespace Comm
{
	/** Handle one line received over USB or UART, or from the simulated Duet: one reply */
	void receiveLine(std::string_view line);

	/** Handle one reply, already read as JSON, telling what it is by what it holds; the reply may be taken from */
	void receiveReply(OM::Node& reply);
} // namespace Comm

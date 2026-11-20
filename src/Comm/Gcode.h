/*
 * Gcode.h
 *
 * Writing text into G-code. RepRapFirmware reads a quoted string parameter with two escapes: a double quote is written
 * twice, and a single quote makes the character after it lower case, so a single quote itself is written twice too.
 * Every file name and other text put into a command goes through quoted(), or a name like "Andy's part.gcode" reaches
 * the printer as "Andys part.gcode".
 */

#pragma once

#include <string>
#include <string_view>

namespace Comm
{
	/** `text` as a quoted G-code string parameter, quotes included, e.g. `"Andy''s part.gcode"` */
	std::string quoted(std::string_view text);
} // namespace Comm

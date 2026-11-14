/*
 * Services.cpp
 */

#include "Services.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include <string>

namespace UI::Xml::Services
{
	static std::function<void(std::string_view)> s_gcodeSender; // Replacement sender, or empty for the Duet

	void sendGcode(std::string_view gcode)
	{
		// Lines are newline-terminated on the wire; XML authors shouldn't have to write "\n"
		std::string line(gcode);
		if (line.empty() || line.back() != '\n')
		{
			line += '\n';
		}

		LOG_DBG("XML UI sending '{:s}'", gcode);
		if (s_gcodeSender)
		{
			s_gcodeSender(line);
			return;
		}
		Comm::DUET.SendGcode(line);
	}

	void setGcodeSender(std::function<void(std::string_view)> sender)
	{
		s_gcodeSender = std::move(sender);
	}

} // namespace UI::Xml::Services

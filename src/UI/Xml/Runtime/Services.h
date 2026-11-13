/*
 * Services.h
 *
 * What the XML UI needs from the rest of the application, behind one small interface so tests can replace it.
 */

#pragma once

#include "UI/Xml/View/NumberEntry.h"
#include <functional>
#include <string_view>

namespace UI::Xml::Services
{
	/**
	 * Ask the user for a number on the shared number pad: the view registered as `numberpad`
	 * (assets/ui/components/modals/numberpad.xml). Replaces any question it was showing.
	 * @return false if there is no number pad
	 */
	bool askNumber(NumberRequest request);

	/**
	 * Ask the user for digits and dots, such as an IP address, on the shared number pad, as askNumber()
	 * @return false if there is no number pad
	 */
	bool askText(TextRequest request);

	/** Send a line of G-code to the printer. A newline is added if missing. */
	void sendGcode(std::string_view gcode);

	/** Send G-code with `sender` instead of the Duet connection (tests); nullptr restores the Duet connection */
	void setGcodeSender(std::function<void(std::string_view)> sender);

} // namespace UI::Xml::Services

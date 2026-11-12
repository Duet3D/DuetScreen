/*
 * Services.h
 *
 * What the XML UI needs from the rest of the application, behind one small interface so tests can replace it.
 */

#pragma once

#include <functional>
#include <string_view>

namespace UI
{
	class ModalNumberPad;
}

namespace UI::Xml::Services
{
	/** Send a line of G-code to the printer. A newline is added if missing. */
	void sendGcode(std::string_view gcode);

	/** Send G-code with `sender` instead of the Duet connection (tests); nullptr restores the Duet connection */
	void setGcodeSender(std::function<void(std::string_view)> sender);

	/** The shared number pad `<ds_slider numberpad="true">` opens, or nullptr if there is none */
	ModalNumberPad* numberPad();

	/** Where numberPad() comes from (the view that owns it); nullptr for none */
	void setNumberPadProvider(std::function<ModalNumberPad*()> provider);
} // namespace UI::Xml::Services

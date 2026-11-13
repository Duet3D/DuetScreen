/*
 * Services.h
 *
 * What the XML UI needs from the rest of the application, behind one small interface so tests can replace it.
 */

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml::Services
{
	/** A number to ask the user for (see askNumber()) */
	struct NumberRequest
	{
		std::string header;						// Shown above the keys, e.g. "T0 heater 1 active"
		float min = 0.0f;						// Smallest value accepted
		float max = 0.0f;						// Largest value accepted
		std::optional<float> value;				// Shown to start with; the pad starts empty if not set
		std::function<void(float)> onConfirm; // Called with the value the user confirms
	};

	/**
	 * Ask the user for a number with the number pad registered as `numberPad` (see Navigation.h), e.g. one a screen
	 * hosts with `<ds_view name="tool_numberpad" view="numberpad"/>`. Presenters use this rather than the widget.
	 * @return false if there is no such number pad
	 */
	bool askNumber(std::string_view numberPad, NumberRequest request);

	/**
	 * Ask the user for a number with the number pad nearest `asker` (the widget the user tapped, ActionArgs::target):
	 * one in the same view before any further out, else the shared one registered as `numberpad`. So a page can
	 * keep the number pad over itself by hosting its own: `<ds_view name="move_numberpad" view="numberpad"/>`.
	 * @return false if there is no number pad
	 */
	bool askNumber(lv_obj_t* asker, NumberRequest request);

	/** Text typed on a number pad, such as an IP address (see askText()) */
	struct TextRequest
	{
		std::string header;								// Shown above the keys
		std::string text;								// Shown to start with
		std::function<void(std::string_view)> onConfirm; // Called with the text the user confirms
	};

	/**
	 * Ask the user for digits and dots, such as an IP address, with the number pad nearest `asker`, found as for
	 * askNumber()
	 * @return false if there is no number pad
	 */
	bool askText(lv_obj_t* asker, TextRequest request);

	/** Send a line of G-code to the printer. A newline is added if missing. */
	void sendGcode(std::string_view gcode);

	/** Send G-code with `sender` instead of the Duet connection (tests); nullptr restores the Duet connection */
	void setGcodeSender(std::function<void(std::string_view)> sender);

} // namespace UI::Xml::Services

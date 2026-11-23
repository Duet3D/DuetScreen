/*
 * Responses.h
 *
 * What the printer answers commands with, as text ("ok", "Error: …", the output of M122, …), shown to the user as a
 * notification and in the console. It arrives as the `resp` of a reply over USB and UART, and from rr_reply or DSF's
 * code endpoint over the network; it is shown the one way, here.
 */

#pragma once

#include <iterator>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

/** How serious a response is, from how the firmware starts it */
enum class ResponseType
{
	INFO = 0,
	SUCCESS,
	WARNING,
	ERROR,
};

/** The translation key of each ResponseType's name */
constexpr std::string_view RESPONSE_TYPE_STRINGS[] = {
	"settings.severity.info",
	"settings.severity.success",
	"settings.severity.warn",
	"settings.severity.error",
};

static_assert(std::size(RESPONSE_TYPE_STRINGS) == magic_enum::enum_count<ResponseType>(),
			  "RESPONSE_TYPE_STRINGS size does not match ResponseType enum");

namespace Comm
{
	/**
	 * Show one response from the printer; trailing line endings are dropped, and nothing is shown if nothing is left
	 * @param text The response; "Error: …" and "Warning: …" are shown as such
	 */
	void showResponse(std::string_view text);
} // namespace Comm

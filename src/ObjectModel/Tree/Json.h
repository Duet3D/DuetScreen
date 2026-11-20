/*
 * Json.h
 *
 * Reading JSON text as a Node. Everything the printer sends is JSON: object model replies (M409, rr_model, the DSF
 * WebSocket), file lists, file information, thumbnails and messages. It is all read here, the one way, into a Node,
 * which keeps each value as the printer sent it (numbers keep their text) and reads safely (a missing field reads as
 * null).
 *
 * The reading is strict JSON: every escape (\uXXXX included, with surrogate pairs) becomes UTF-8, and anything that
 * isn't JSON is an error, so a reply cut short or garbled is never half applied.
 */

#pragma once

#include "Node.h"
#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

namespace OM
{
	/** Why JSON text couldn't be read */
	struct JsonError
	{
		size_t position = 0; // Byte offset the reading stopped at
		std::string message; // What was wrong there
	};

	/** The deepest nesting of objects and arrays read; deeper text is an error rather than a risk to the stack */
	constexpr size_t maxJsonDepth = 64;

	/**
	 * Read `text` as one JSON value
	 * @return the value as a Node, or why it isn't JSON
	 */
	std::expected<Node, JsonError> parseJson(std::string_view text);
} // namespace OM

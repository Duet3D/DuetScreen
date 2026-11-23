/*
 * Lines.h
 *
 * Splitting text into lines, the one way: G-code to send, a file to write a line at a time, the responses rr_reply
 * gathers.
 */

#pragma once

#include <string_view>

namespace Comm
{
	/** Whether forEachLine() hands on empty lines */
	enum class EmptyLines
	{
		skip, // An empty line says nothing (G-code, responses)
		keep, // An empty line is part of the text (a file's contents)
	};

	/**
	 * Call `each(line)` for each line of `text`, split at '\n', each without its line ending ('\r' included)
	 * @param empty Whether empty lines are handed on
	 */
	template <typename Each>
	void forEachLine(std::string_view text, EmptyLines empty, Each&& each)
	{
		while (true)
		{
			const size_t end = text.find('\n');
			std::string_view line = text.substr(0, end);
			if (!line.empty() && line.back() == '\r')
			{
				line.remove_suffix(1);
			}
			if (!line.empty() || empty == EmptyLines::keep)
			{
				each(line);
			}
			if (end == std::string_view::npos)
			{
				return;
			}
			text.remove_prefix(end + 1);
			if (text.empty())
			{
				return; // a final line ending ends the last line, it doesn't start another
			}
		}
	}
} // namespace Comm

/*
 * OmName.h
 *
 * A name XML uses for object model data, taken apart:
 *
 *     om.heat.heaters[1].current|fixed:1
 *     └┬┘└────────┬────────────┘ └─┬─┘ └┬┘
 *   absolute     path       converter  argument
 *
 * Without `om.` the path is relative, as in a row of an object model list (`current` in a row of `om.heat.heaters`).
 * The converter, if any, says what the subject holds (see Converters.h); without one it holds the value as text.
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml
{
	struct OmName
	{
		/** The prefix of an absolute name */
		static constexpr std::string_view PREFIX = "om.";

		/**
		 * Take `name` apart
		 * @return the parts, or nothing if `name` is empty, or has an empty path or converter
		 */
		static std::optional<OmName> parse(std::string_view name);

		bool absolute = false;	// Whether it starts with `om.`
		std::string path;		// The path, without `om.`, as written, e.g. "heat.heaters[1].current"
		std::string converter;	// The converter's name, or empty for none
		std::string argument;	// The converter's argument, or empty for none
	};
} // namespace UI::Xml

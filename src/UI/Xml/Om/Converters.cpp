/*
 * Converters.cpp
 */

#include "Converters.h"
#include "UI/Xml/Runtime/Translations.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fmt/format.h>
#include <limits>
#include <optional>

namespace UI::Xml
{
	namespace
	{
		/** Whether the node holds something that reads as a number */
		bool isNumeric(const OM::Node& node)
		{
			return !std::isnan(node.number(std::nan("")));
		}

		/** Read `text` as a number, or `fallback` if it isn't wholly one */
		double toDouble(std::string_view text, double fallback)
		{
			double value = 0;
			const char* last = text.data() + text.size();
			const auto [ptr, error] = std::from_chars(text.data(), last, value);
			return (error == std::errc() && ptr == last && !text.empty()) ? value : fallback;
		}

		/** Clamp a rounded number into an int subject's range */
		int32_t toInt32(double value)
		{
			if (std::isnan(value))
			{
				return 0;
			}
			const double rounded = std::round(value);
			if (rounded >= static_cast<double>(std::numeric_limits<int32_t>::max()))
			{
				return std::numeric_limits<int32_t>::max();
			}
			if (rounded <= static_cast<double>(std::numeric_limits<int32_t>::min()))
			{
				return std::numeric_limits<int32_t>::min();
			}
			return static_cast<int32_t>(rounded);
		}

		/** The value as text, as received */
		std::string asText(const OM::Node& node, const OM::Path&, std::string_view)
		{
			return std::string(node.text());
		}

		/** The value rounded to a whole number */
		int32_t asInteger(const OM::Node& node, const OM::Path&, std::string_view)
		{
			return toInt32(node.number(0));
		}

		/** The value × 100, rounded */
		int32_t asPercent(const OM::Node& node, const OM::Path&, std::string_view)
		{
			return toInt32(node.number(0) * 100);
		}

		/** The value × the argument, rounded */
		int32_t scaled(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			return toInt32(node.number(0) * toDouble(argument, 1));
		}

		/** The value with the argument's number of decimals */
		std::string fixed(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			if (!isNumeric(node))
			{
				return "";
			}
			const int decimals = std::clamp(static_cast<int>(toDouble(argument, 0)), 0, 9);
			return fmt::format("{:.{}f}", node.number(0), decimals);
		}

		/** An array's length */
		int32_t count(const OM::Node& node, const OM::Path&, std::string_view)
		{
			return node.isArray() ? static_cast<int32_t>(node.size()) : 0;
		}

		/** 1 if there and not null */
		int32_t exists(const OM::Node& node, const OM::Path&, std::string_view)
		{
			return node.exists() ? 1 : 0;
		}

		/** 1 if the value, as text, is the argument */
		int32_t equals(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			return node.isValue() && node.text() == argument ? 1 : 0;
		}

		/** Seconds as h:mm:ss, or m:ss under an hour */
		std::string duration(const OM::Node& node, const OM::Path&, std::string_view)
		{
			if (!isNumeric(node))
			{
				return "";
			}
			const auto total = static_cast<int64_t>(std::max(0.0, std::round(node.number(0))));
			const int64_t hours = total / 3600;
			const int64_t minutes = (total / 60) % 60;
			const int64_t seconds = total % 60;
			return hours > 0 ? fmt::format("{:d}:{:02d}:{:02d}", hours, minutes, seconds)
							 : fmt::format("{:d}:{:02d}", minutes, seconds);
		}

		/** A size in bytes, as B, KB, MB or GB */
		std::string bytes(const OM::Node& node, const OM::Path&, std::string_view)
		{
			if (!isNumeric(node))
			{
				return "";
			}
			static constexpr std::array<std::string_view, 4> units = {"B", "KB", "MB", "GB"};
			double value = node.number(0);
			size_t unit = 0;
			while (std::abs(value) >= 1024 && unit + 1 < units.size())
			{
				value /= 1024;
				unit++;
			}
			return unit == 0 ? fmt::format("{:.0f} {:s}", value, units[unit])
							 : fmt::format("{:.1f} {:s}", value, units[unit]);
		}

		/** The number of the item the field belongs to: the last index in its path, or nothing if there is none */
		std::optional<size_t> itemNumber(const OM::Path& path)
		{
			for (auto it = path.segments().rbegin(); it != path.segments().rend(); ++it)
			{
				if (const auto* index = std::get_if<size_t>(&*it))
				{
					return *index;
				}
			}
			return std::nullopt;
		}

		/** The value, or if it is empty, the translation of the argument with the item's number */
		std::string orDefault(const OM::Node& node, const OM::Path& path, std::string_view argument)
		{
			if (node.isValue() && !node.text().empty())
			{
				return std::string(node.text());
			}
			const auto tag = Translations::key(argument);
			if (!tag)
			{
				return "";
			}
			const std::string key(*tag);
			const std::string* translation = i18n::find(key);
			if (translation == nullptr || *translation == key)
			{
				return "";
			}
			const auto number = itemNumber(path);
			return number ? fmt::format(fmt::runtime(*translation), *number) : *translation;
		}

		/** The translation of "<argument>.<value>" (the argument written `tr.<key>`), or the value if there is none */
		std::string translated(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			if (!node.isValue())
			{
				return "";
			}
			const auto prefix = Translations::key(argument);
			if (!prefix)
			{
				return std::string(node.text());
			}
			const std::string key = fmt::format("{:s}.{:s}", *prefix, node.text());
			const std::string* translation = i18n::find(key);
			// translate() files a missing key as its own translation; that isn't one
			return translation != nullptr && *translation != key ? *translation : std::string(node.text());
		}

		// The converters, the default first
		const std::array s_converters = {
			Converter{.name = "",
					  .type = Converter::Type::text,
					  .description = "The value as text, as the printer sent it; empty for null, an object or an array",
					  .argument = "",
					  .toInteger = nullptr,
					  .toText = asText},
			Converter{.name = "int",
					  .type = Converter::Type::integer,
					  .description = "The value rounded to a whole number: a bool is 1 or 0, anything else not a "
									 "number is 0",
					  .argument = "",
					  .toInteger = asInteger,
					  .toText = nullptr},
			Converter{.name = "percent",
					  .type = Converter::Type::integer,
					  .description = "The value × 100, rounded, for a fraction such as a fan's speed (0-1)",
					  .argument = "",
					  .toInteger = asPercent,
					  .toText = nullptr},
			Converter{.name = "scale",
					  .type = Converter::Type::integer,
					  .description = "The value × the argument, rounded, e.g. `scale:10` for tenths",
					  .argument = "The factor",
					  .toInteger = scaled,
					  .toText = nullptr},
			Converter{.name = "fixed",
					  .type = Converter::Type::text,
					  .description = "A number with the argument's number of decimals, e.g. `fixed:1` gives 61.2; "
									 "empty for anything not a number",
					  .argument = "Decimals, 0-9",
					  .toInteger = nullptr,
					  .toText = fixed},
			Converter{.name = "count",
					  .type = Converter::Type::integer,
					  .description = "The number of items of an array; 0 for anything else",
					  .argument = "",
					  .toInteger = count,
					  .toText = nullptr},
			Converter{.name = "exists",
					  .type = Converter::Type::integer,
					  .description = "1 if the field is there and not null, else 0",
					  .argument = "",
					  .toInteger = exists,
					  .toText = nullptr},
			Converter{.name = "eq",
					  .type = Converter::Type::integer,
					  .description = "1 if the value is the argument, else 0, e.g. `state|eq:fault`",
					  .argument = "The value compared with",
					  .toInteger = equals,
					  .toText = nullptr},
			Converter{.name = "duration",
					  .type = Converter::Type::text,
					  .description = "Seconds as h:mm:ss, or m:ss under an hour",
					  .argument = "",
					  .toInteger = nullptr,
					  .toText = duration},
			Converter{.name = "bytes",
					  .type = Converter::Type::text,
					  .description = "A number of bytes as B, KB, MB or GB",
					  .argument = "",
					  .toInteger = nullptr,
					  .toText = bytes},
			Converter{.name = "default",
					  .type = Converter::Type::text,
					  .description = "The value, or if it is empty, the translation of the argument with the item's "
									 "number (the last index in the path): `name|default:tr.fine_tune.fan` gives "
									 "\"Fan 2\" for fan 2 if it has no name",
					  .argument = "The translation key of the default, `tr.<key>`; `{:d}` in it is the number",
					  .toInteger = nullptr,
					  .toText = orDefault,
					  .argumentKind = Converter::Argument::translationKey},
			Converter{.name = "translate",
					  .type = Converter::Type::text,
					  .description = "The translation of `<argument>.<value>`, or the value if there is none, e.g. "
									 "`state|translate:tr.heater.state`",
					  .argument = "The translation key the value is added to, `tr.<key>`",
					  .toInteger = nullptr,
					  .toText = translated,
					  .argumentKind = Converter::Argument::translationGroup},
		};
	} // namespace

	const Converter* findConverter(std::string_view name)
	{
		for (const auto& converter : s_converters)
		{
			if (converter.name == name)
			{
				return &converter;
			}
		}
		return nullptr;
	}

	std::span<const Converter> allConverters()
	{
		return s_converters;
	}
} // namespace UI::Xml

/*
 * Converters.cpp
 */

#include "Converters.h"
#include "UI/Xml/Data/PrinterModel.h"
#include "UI/Xml/Runtime/Translations.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fmt/format.h>
#include <limits>
#include <optional>
#include <variant>

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

		/** The field of `node` at the dotted path `field`, e.g. "thermostatic.lowTemperature" */
		const OM::Node& fieldOf(const OM::Node& node, std::string_view field)
		{
			const OM::Node* at = &node;
			while (!field.empty())
			{
				const size_t dot = field.find('.');
				at = &(*at)[field.substr(0, dot)];
				field = dot == std::string_view::npos ? std::string_view() : field.substr(dot + 1);
			}
			return *at;
		}

		/** How many items of an array have the field `field` (there and not null) */
		int32_t countHaving(const OM::Node& node, const OM::Path&, std::string_view field)
		{
			int32_t having = 0;
			for (size_t i = 0; node.isArray() && i < node.size(); ++i)
			{
				having += node[i].exists() && fieldOf(node[i], field).exists() ? 1 : 0;
			}
			return having;
		}

		/** How many items of an array lack the field `field` (missing or null); a null item counts as neither */
		int32_t countLacking(const OM::Node& node, const OM::Path&, std::string_view field)
		{
			int32_t lacking = 0;
			for (size_t i = 0; node.isArray() && i < node.size(); ++i)
			{
				lacking += node[i].exists() && !fieldOf(node[i], field).exists() ? 1 : 0;
			}
			return lacking;
		}

		/** The tools whose list `list` holds the number of the item at `path`, e.g. a fan's (`fans[1]`) */
		std::string toolsUsing(const OM::Node&, const OM::Path& path, std::string_view list)
		{
			const auto& segments = path.segments();
			const auto* number = segments.empty() ? nullptr : std::get_if<size_t>(&segments.back());
			return number != nullptr ? PrinterModel::toolsUsing(list, *number) : std::string();
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

		/** The translation of `tag` (`tr.<key>`) with `number` in it (if any); empty if there is none */
		std::string translatedWith(std::string_view tag, std::optional<int64_t> number)
		{
			const auto key = Translations::key(tag);
			if (!key)
			{
				return "";
			}
			const std::string name(*key);
			const std::string* translation = i18n::find(name);
			if (translation == nullptr || *translation == name)
			{
				return "";
			}
			return number ? fmt::format(fmt::runtime(*translation), *number) : *translation;
		}

		/**
		 * The value, or if it is empty, the translation of the argument (`tr.<key>`, or `tr.<key>:<number>`) with the
		 * number given, else the item's number
		 */
		std::string orDefault(const OM::Node& node, const OM::Path& path, std::string_view argument)
		{
			if (node.isValue() && !node.text().empty())
			{
				return std::string(node.text());
			}
			const size_t colon = argument.find(':');
			if (colon != std::string_view::npos)
			{
				const double number = toDouble(argument.substr(colon + 1), std::nan(""));
				return translatedWith(argument.substr(0, colon),
									  std::isnan(number) ? std::nullopt
														 : std::optional(static_cast<int64_t>(std::round(number))));
			}
			const auto number = itemNumber(path);
			return translatedWith(argument, number ? std::optional(static_cast<int64_t>(*number)) : std::nullopt);
		}

		/** The translation of the argument (`tr.<key>`) with the item's number in it; empty if there is none */
		std::string numbered(const OM::Node&, const OM::Path& path, std::string_view argument)
		{
			const auto number = itemNumber(path);
			return number ? translatedWith(argument, static_cast<int64_t>(*number)) : "";
		}

		/** The value rounded down, but not below the argument */
		int32_t atLeast(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			return toInt32(std::max(std::floor(node.number(0)), toDouble(argument, 0)));
		}

		/** The value rounded up, but not above the argument */
		int32_t atMost(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			return toInt32(std::min(std::ceil(node.number(0)), toDouble(argument, 0)));
		}

		/** The value rounded, kept between the argument's two numbers (`<min>:<max>`) */
		int32_t clamped(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			const size_t colon = argument.find(':');
			const double low = toDouble(argument.substr(0, colon), 0);
			const double high = colon == std::string_view::npos ? low : toDouble(argument.substr(colon + 1), low);
			return toInt32(std::clamp(std::round(node.number(0)), std::min(low, high), std::max(low, high)));
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

		/** The translation of the argument (`tr.<key>`) with the value as a whole number in it; empty for a value that
		 * isn't a number, or is below 0, which RepRapFirmware uses for none (a fan's `rpm` without a tachometer) */
		std::string formatted(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			if (!isNumeric(node) || node.number(0) < 0)
			{
				return "";
			}
			return translatedWith(argument, static_cast<int64_t>(std::round(node.number(0))));
		}

		/** The smallest multiple of the argument above the value, and at least the argument: a scale's top */
		int32_t scaleTop(const OM::Node& node, const OM::Path&, std::string_view argument)
		{
			const double step = std::max(1.0, toDouble(argument, 100));
			return toInt32(std::max(step, (std::floor(node.number(0) / step) + 1) * step));
		}

		/** A heater's (`heat.heaters[n]`) target: its active temperature while active, its standby one on standby */
		double heaterTargetOf(const OM::Node& heater)
		{
			const std::string_view state = heater["state"].text();
			return state == "active"	? heater["active"].number(0)
				   : state == "standby" ? heater["standby"].number(0)
										: 0;
		}

		/** A heater's target, else 0 (see heaterTargetOf()) */
		int32_t heaterTarget(const OM::Node& node, const OM::Path&, std::string_view)
		{
			return toInt32(heaterTargetOf(node));
		}

		/** "Target 215°C" while a heater heats to a target, else its state translated, e.g. "Off" or "Fault" */
		std::string heaterDetail(const OM::Node& node, const OM::Path&, std::string_view)
		{
			if (!node.isObject())
			{
				return "";
			}
			const int32_t target = toInt32(heaterTargetOf(node));
			if (target > 0)
			{
				return _("widget.gauge_target", target);
			}
			const std::string key = fmt::format("temperature.status.{:s}", node["state"].text());
			const std::string* translation = i18n::find(key);
			return translation != nullptr && *translation != key ? *translation : std::string(node["state"].text());
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
			Converter{.name = "count_having",
					  .type = Converter::Type::integer,
					  .description =
						  "How many items of an array have a field (there and not null), e.g. "
						  "`om.fans|count_having:thermostatic.lowTemperature` for the fans the firmware runs",
					  .argument = "The field, a dotted path from an item, e.g. `thermostatic.lowTemperature`",
					  .toInteger = countHaving,
					  .toText = nullptr},
			Converter{.name = "count_lacking",
					  .type = Converter::Type::integer,
					  .description = "How many items of an array lack a field (missing or null), e.g. "
									 "`om.fans|count_lacking:thermostatic.lowTemperature` for the fans you set",
					  .argument = "The field, a dotted path from an item, e.g. `thermostatic.lowTemperature`",
					  .toInteger = countLacking,
					  .toText = nullptr},
			Converter{.name = "tools_using",
					  .type = Converter::Type::text,
					  .description = "Of an item of a list, e.g. `fans[1]`: the names of the tools whose list (the "
									 "argument) holds its number, e.g. `om.fans[1]|tools_using:fans` gives "
									 "\"Revo, Tool 2\"; empty if none",
					  .argument = "The tools' list, e.g. `fans` or `extruders`",
					  .toInteger = nullptr,
					  .toText = toolsUsing},
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
					  .description =
						  "The value, or if it is empty, the translation of the argument with a number: the one after "
						  "the key, else the item's (the last index in the path). `row.name|default:tr.fine_tune.fan` "
						  "gives \"Fan 2\" for fan 2 if it has no name; "
						  "`om.sensors.analog[{om.heat.heaters[{row.value}].sensor}].name|default:tr.temperature."
						  "heater:{row.value}` gives \"Heater 2\" for heater 2 if its sensor has no name",
					  .argument = "The translation key of the default, `tr.<key>`, or `tr.<key>:<number>` (the "
								  "number may be a subject's, `{name}`); `{:d}` in the translation is the number",
					  .toInteger = nullptr,
					  .toText = orDefault,
					  .argumentKind = Converter::Argument::translationKey},
			Converter{.name = "numbered",
					  .type = Converter::Type::text,
					  .description =
						  "The translation of the argument with the item's number (the last index in the path) "
						  "in it, whatever the value: `row.value|numbered:tr.temperature.bed_number` in a row "
						  "of `om.heat.bedHeaters` gives \"Bed 0\" for bed 0",
					  .argument = "The translation key, `tr.<key>`; `{:d}` in the translation is the number",
					  .toInteger = nullptr,
					  .toText = numbered,
					  .argumentKind = Converter::Argument::translationKey},
			Converter{.name = "atleast",
					  .type = Converter::Type::integer,
					  .description = "The value rounded down, but not below the argument, e.g. a bar's bottom: "
									 "`om.heat.heaters[1].min|atleast:0`",
					  .argument = "The least it gives",
					  .toInteger = atLeast,
					  .toText = nullptr},
			Converter{.name = "atmost",
					  .type = Converter::Type::integer,
					  .description = "The value rounded up, but not above the argument",
					  .argument = "The most it gives",
					  .toInteger = atMost,
					  .toText = nullptr},
			Converter{.name = "clamp",
					  .type = Converter::Type::integer,
					  .description = "The value rounded, kept between the argument's two numbers, e.g. `|clamp:0:100`",
					  .argument = "The least and the most it gives, `<min>:<max>`",
					  .toInteger = clamped,
					  .toText = nullptr},
			Converter{.name = "translate",
					  .type = Converter::Type::text,
					  .description = "The translation of `<argument>.<value>`, or the value if there is none, e.g. "
									 "`state|translate:tr.heater.state`",
					  .argument = "The translation key the value is added to, `tr.<key>`",
					  .toInteger = nullptr,
					  .toText = translated,
					  .argumentKind = Converter::Argument::translationGroup},
			Converter{.name = "format",
					  .type = Converter::Type::text,
					  .description =
						  "The translation of the argument with the value, rounded, as its number; empty for "
						  "anything not a number or below 0 (RepRapFirmware's \"none\"), e.g. "
						  "`om.fans[0].rpm|format:tr.widget.gauge_rpm` gives \"1200 RPM\", or nothing for a "
						  "fan without a tachometer",
					  .argument = "The translation key, `tr.<key>`; `{:d}` in the translation is the number",
					  .toInteger = nullptr,
					  .toText = formatted,
					  .argumentKind = Converter::Argument::translationKey},
			Converter{.name = "scale_top",
					  .type = Converter::Type::integer,
					  .description =
						  "The smallest multiple of the argument above the value, and at least the argument: "
						  "the top of a scale for a value with no known range, e.g. a sensor's reading of 25 "
						  "gives 100 with `scale_top:100`, and 130 gives 200",
					  .argument = "The step, e.g. 100",
					  .toInteger = scaleTop,
					  .toText = nullptr},
			Converter{.name = "heater_target",
					  .type = Converter::Type::integer,
					  .description = "Of a heater, `heat.heaters[n]`: what it heats to, its active temperature while "
									 "active and its standby one on standby, else 0, e.g. "
									 "`om.heat.heaters[1]|heater_target`",
					  .argument = "",
					  .toInteger = heaterTarget,
					  .toText = nullptr},
			Converter{.name = "heater_detail",
					  .type = Converter::Type::text,
					  .description =
						  "Of a heater, `heat.heaters[n]`: \"Target 215°C\" while it heats to a target, else "
						  "its state, translated, e.g. \"Off\" or \"Fault\"",
					  .argument = "",
					  .toInteger = nullptr,
					  .toText = heaterDetail},
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

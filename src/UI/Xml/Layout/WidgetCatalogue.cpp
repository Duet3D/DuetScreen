/*
 * WidgetCatalogue.cpp
 */

#include "WidgetCatalogue.h"
#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/Globals.h"
#include "UI/Xml/Binding/List.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <fmt/format.h>
#include <utility>

namespace UI::Xml
{
	namespace
	{
		/** Each kind with its name in a widget file */
		constexpr std::array<std::pair<SettingKind, std::string_view>, 5> KIND_NAMES = {{
			{SettingKind::Int, "int"},
			{SettingKind::Bool, "bool"},
			{SettingKind::Text, "text"},
			{SettingKind::Choice, "choice"},
			{SettingKind::Row, "row"},
		}};

		std::map<std::string, WidgetDef, std::less<>> s_widgets; // The pool, by id

		/** The kind called `name`, or nullopt */
		std::optional<SettingKind> kindNamed(std::string_view name)
		{
			for (const auto& [kind, kindName] : KIND_NAMES)
			{
				if (kindName == name)
				{
					return kind;
				}
			}
			return std::nullopt;
		}

		/** `json[key]` if it is a string, else `fallback`; sets `error` if it is there but isn't a string */
		std::string stringOf(const nlohmann::json& json, const char* key, std::string& error, std::string fallback = {})
		{
			const auto it = json.find(key);
			if (it == json.end())
			{
				return fallback;
			}
			if (!it->is_string())
			{
				error = fmt::format("\"{:s}\" must be text", key);
				return fallback;
			}
			return it->get<std::string>();
		}

		/** Read a [width, height] size in pixels; sets `error` if it is there but isn't one */
		Size sizeOf(const nlohmann::json& json, const char* key, Size fallback, std::string& error)
		{
			const auto it = json.find(key);
			if (it == json.end())
			{
				return fallback;
			}
			if (!it->is_array() || it->size() != 2 || !(*it)[0].is_number_integer() || !(*it)[1].is_number_integer() ||
				(*it)[0].get<int32_t>() < 1 || (*it)[1].get<int32_t>() < 1)
			{
				error = fmt::format("size \"{:s}\" must be [width, height] in pixels, each at least 1", key);
				return fallback;
			}
			return {(*it)[0].get<int32_t>(), (*it)[1].get<int32_t>()};
		}

		/** Text as a whole number, or nullopt */
		std::optional<int32_t> parseInt(std::string_view text)
		{
			int32_t value = 0;
			const char* end = text.data() + text.size();
			const auto [ptr, error] = std::from_chars(text.data(), end, value);
			if (error != std::errc() || ptr != end)
			{
				return std::nullopt;
			}
			return value;
		}

		/** Read one setting; sets `error` if it isn't a valid one */
		SettingDef parseSetting(const nlohmann::json& json, std::string& error)
		{
			SettingDef setting{.name = {}, .kind = SettingKind::Text};
			if (!json.is_object())
			{
				error = "each setting must be an object";
				return setting;
			}
			setting.name = stringOf(json, "name", error);
			if (setting.name.empty())
			{
				error = "a setting has no \"name\"";
				return setting;
			}
			const std::string kind = stringOf(json, "kind", error);
			if (auto parsed = kindNamed(kind))
			{
				setting.kind = *parsed;
			}
			else
			{
				error = fmt::format("setting \"{:s}\": unknown kind \"{:s}\"", setting.name, kind);
				return setting;
			}
			setting.title = stringOf(json, "title", error, setting.name);
			setting.titleTag = stringOf(json, "title_tag", error);
			setting.unit = stringOf(json, "unit", error);

			switch (setting.kind)
			{
			case SettingKind::Int:
				setting.min = json.value("min", setting.min);
				setting.max = json.value("max", setting.max);
				if (setting.min > setting.max)
				{
					error = fmt::format("setting \"{:s}\": min is above max", setting.name);
				}
				break;
			case SettingKind::Text:
				setting.maxLength = json.value("max_length", setting.maxLength);
				break;
			case SettingKind::Choice:
				for (const auto& option : json.value("options", nlohmann::json::array()))
				{
					if (!option.is_object() || !option.contains("value"))
					{
						error = fmt::format("setting \"{:s}\": each option needs a \"value\"", setting.name);
						return setting;
					}
					const std::string value = SettingDef::toProp(option["value"]);
					setting.options.push_back(
						{value, stringOf(option, "title", error, value), stringOf(option, "title_tag", error)});
				}
				if (setting.options.empty())
				{
					error = fmt::format("setting \"{:s}\": a choice needs \"options\"", setting.name);
				}
				break;
			case SettingKind::Row:
				setting.list = stringOf(json, "list", error);
				setting.key = stringOf(json, "key", error);
				setting.label = stringOf(json, "label", error, setting.key);
				if (setting.list.empty() || setting.key.empty())
				{
					error = fmt::format("setting \"{:s}\": a row setting needs \"list\" and \"key\"", setting.name);
				}
				break;
			case SettingKind::Bool:
				break;
			}

			if (!json.contains("default"))
			{
				error = fmt::format("setting \"{:s}\" has no \"default\"", setting.name);
				return setting;
			}
			auto fallback = setting.accept(json["default"]);
			if (!fallback)
			{
				error = fmt::format("setting \"{:s}\": the default isn't one of its values", setting.name);
				return setting;
			}
			setting.fallback = std::move(*fallback);
			return setting;
		}
	} // namespace

	std::optional<nlohmann::json> SettingDef::accept(const nlohmann::json& value) const
	{
		switch (kind)
		{
		case SettingKind::Int:
		{
			std::optional<int32_t> number;
			if (value.is_number_integer())
			{
				number = value.get<int32_t>();
			}
			else if (value.is_string())
			{
				number = parseInt(value.get<std::string>());
			}
			if (!number || *number < min || *number > max)
			{
				return std::nullopt;
			}
			return nlohmann::json(*number);
		}
		case SettingKind::Bool:
			if (value.is_boolean())
			{
				return value;
			}
			if (value.is_string() && (value == "true" || value == "false"))
			{
				return nlohmann::json(value == "true");
			}
			return std::nullopt;
		case SettingKind::Text:
			if (!value.is_string() || value.get<std::string>().size() > maxLength)
			{
				return std::nullopt;
			}
			return value;
		case SettingKind::Choice:
		{
			if (!value.is_string() && !value.is_number_integer())
			{
				return std::nullopt;
			}
			const std::string text = toProp(value);
			const bool known = std::any_of(
				options.begin(), options.end(), [&text](const SettingOption& option) { return option.value == text; });
			return known ? std::optional(nlohmann::json(text)) : std::nullopt;
		}
		case SettingKind::Row:
			if (value.is_string() || value.is_number_integer())
			{
				return value;
			}
			return std::nullopt;
		}
		return std::nullopt;
	}

	std::string SettingDef::toProp(const nlohmann::json& value)
	{
		if (value.is_string())
		{
			return value.get<std::string>();
		}
		if (value.is_boolean())
		{
			return value.get<bool>() ? "true" : "false";
		}
		return value.dump();
	}

	std::vector<SettingOption> SettingDef::choices() const
	{
		if (kind == SettingKind::Choice)
		{
			return options;
		}
		std::vector<SettingOption> rows;
		ListBase* rowList = kind == SettingKind::Row ? Globals::findList(nullptr, list) : nullptr;
		for (size_t i = 0; rowList != nullptr && i < rowList->size(); i++)
		{
			FieldFinder keyField(key);
			FieldFinder labelField(label);
			rowList->row(i).allFields(keyField);
			rowList->row(i).allFields(labelField);
			if (keyField.foundSubject != nullptr)
			{
				const std::string value = keyField.foundSubject->text();
				rows.push_back({.value = value,
								.title = labelField.foundSubject ? labelField.foundSubject->text() : value,
								.titleTag = ""});
			}
		}
		return rows;
	}

	const SettingDef* WidgetDef::setting(std::string_view name) const
	{
		const auto it =
			std::find_if(settings.begin(), settings.end(), [name](const SettingDef& def) { return def.name == name; });
		return it == settings.end() ? nullptr : &*it;
	}

	bool WidgetDef::fits(std::string_view slot) const
	{
		return std::find(slots.begin(), slots.end(), slot) != slots.end();
	}

	std::map<std::string, std::string> WidgetDef::propsFor(const nlohmann::json& values) const
	{
		std::map<std::string, std::string> result = props;
		for (const auto& setting : settings)
		{
			std::optional<nlohmann::json> value;
			if (values.is_object() && values.contains(setting.name))
			{
				value = setting.accept(values[setting.name]);
			}
			result[setting.name] = SettingDef::toProp(value.value_or(setting.fallback));
		}
		return result;
	}

	namespace WidgetCatalogue
	{
		std::optional<WidgetDef> parse(const nlohmann::json& json, std::string id, std::string& error)
		{
			error.clear();
			if (!json.is_object())
			{
				error = "isn't a JSON object";
				return std::nullopt;
			}

			WidgetDef widget;
			widget.id = std::move(id);
			widget.component = stringOf(json, "component", error, "widget_" + widget.id);
			widget.title = stringOf(json, "title", error, widget.id);
			widget.titleTag = stringOf(json, "title_tag", error);
			widget.description = stringOf(json, "description", error);
			widget.descriptionTag = stringOf(json, "description_tag", error);
			widget.category = stringOf(json, "category", error);
			widget.categoryTag = stringOf(json, "category_tag", error);
			widget.icon = stringOf(json, "icon", error);
			if (!error.empty())
			{
				return std::nullopt;
			}

			const auto slots = json.find("slots");
			if (slots == json.end() || !slots->is_array() || slots->empty() ||
				!std::all_of(slots->begin(), slots->end(), [](const nlohmann::json& s) { return s.is_string(); }))
			{
				error = "\"slots\" must list the layout slots it can be placed in";
				return std::nullopt;
			}
			widget.slots = slots->get<std::vector<std::string>>();

			const nlohmann::json size = json.value("size", nlohmann::json::object());
			widget.minSize = sizeOf(size, "min", widget.minSize, error);
			widget.maxSize = sizeOf(size, "max", widget.maxSize, error);
			widget.defaultSize = sizeOf(size, "default", widget.minSize, error);
			if (!error.empty())
			{
				return std::nullopt;
			}
			const auto within = [&widget](const Size& size)
			{
				return size.width >= widget.minSize.width && size.height >= widget.minSize.height &&
					   size.width <= widget.maxSize.width && size.height <= widget.maxSize.height;
			};
			if (!within(widget.defaultSize))
			{
				error = "the default size must be between the min and max sizes";
				return std::nullopt;
			}

			const nlohmann::json props = json.value("props", nlohmann::json::object());
			if (!props.is_object())
			{
				error = "\"props\" must be an object";
				return std::nullopt;
			}
			for (const auto& [name, value] : props.items())
			{
				widget.props[name] = SettingDef::toProp(value);
			}

			for (const auto& entry : json.value("settings", nlohmann::json::array()))
			{
				SettingDef setting = parseSetting(entry, error);
				if (!error.empty())
				{
					return std::nullopt;
				}
				if (widget.setting(setting.name) != nullptr || widget.props.contains(setting.name))
				{
					error = fmt::format("setting \"{:s}\" is given twice", setting.name);
					return std::nullopt;
				}
				widget.settings.push_back(std::move(setting));
			}
			return widget;
		}

		void set(std::vector<WidgetDef> widgets)
		{
			s_widgets.clear();
			for (auto& widget : widgets)
			{
				std::string id = widget.id;
				s_widgets.insert_or_assign(std::move(id), std::move(widget));
			}
		}

		const WidgetDef* find(std::string_view id)
		{
			const auto it = s_widgets.find(id);
			return it == s_widgets.end() ? nullptr : &it->second;
		}

		const std::map<std::string, WidgetDef, std::less<>>& all()
		{
			return s_widgets;
		}

		std::string_view kindName(SettingKind kind)
		{
			for (const auto& [k, name] : KIND_NAMES)
			{
				if (k == kind)
				{
					return name;
				}
			}
			return "?";
		}
	} // namespace WidgetCatalogue
} // namespace UI::Xml

/*
 * ApiLvgl.cpp
 */

#include "ApiLvgl.h"
#include "XmlTree.h"
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>

namespace UI::Xml::Api
{
	namespace
	{
		/** The value of attribute `name` of `element`, or "" */
		std::string attrOf(const Element& element, std::string_view name)
		{
			const std::string* value = element.attr(name);
			return value ? *value : std::string();
		}

		/**
		 * The attributes a `<prop>` or `<arg>` describes. A `<prop>` made of `<param>`s takes its first param's type,
		 * and each further param is an attribute of its own, `<prop>-<param>`. A `style_x-selector` prop is written
		 * `style_x`, or with a selector after a dash.
		 */
		void addAttributes(const Element& described, std::vector<LvglAttribute>& attributes)
		{
			std::string name = attrOf(described, "name");
			if (name.empty())
			{
				return;
			}
			LvglAttribute attribute{.name = name, .type = attrOf(described, "type"), .help = attrOf(described, "help")};
			constexpr std::string_view selectorSuffix = "-selector";
			if (name.ends_with(selectorSuffix))
			{
				attribute.name = name.substr(0, name.size() - selectorSuffix.size());
				attribute.help += ". Add a part or state after a dash to style only that, e.g. `-pressed`";
			}

			std::vector<const Element*> params;
			for (const auto& child : described.children)
			{
				if (child.tag == "param")
				{
					params.push_back(&child);
				}
			}
			if (!params.empty() && attribute.type.empty())
			{
				attribute.type = attrOf(*params.front(), "type");
			}
			attributes.push_back(std::move(attribute));
			for (std::size_t i = 1; i < params.size(); ++i)
			{
				attributes.push_back({.name = fmt::format("{:s}-{:s}", name, attrOf(*params[i], "name")),
									  .type = attrOf(*params[i], "type"),
									  .help = attrOf(*params[i], "help")});
			}
		}

		/** Read the `<api>` of the description of `widget` ("globals" describes no widget) into `api` */
		void readDescription(const Element& description, const std::string& widget, LvglApi& api)
		{
			const bool isWidget = widget != "globals";
			LvglElement element{.name = widget, .isObject = true};
			for (const auto& item : description.children)
			{
				if (item.tag == "enumdef")
				{
					LvglEnum& type = api.enums.emplace_back(LvglEnum{.name = attrOf(item, "name")});
					for (const auto& value : item.children)
					{
						if (value.tag == "enum")
						{
							type.values.emplace_back(attrOf(value, "name"), attrOf(value, "help"));
						}
					}
				}
				else if (item.tag == "styledef")
				{
					for (const auto& property : item.children)
					{
						if (property.tag == "prop")
						{
							addAttributes(property, api.styleProperties);
						}
					}
				}
				else if (item.tag == "prop" && isWidget)
				{
					addAttributes(item, element.attributes);
				}
				else if (item.tag == "element" && isWidget)
				{
					LvglElement& child = api.elements.emplace_back(
						LvglElement{.name = fmt::format("{:s}-{:s}", widget, attrOf(item, "name")),
									.help = attrOf(item, "help"),
									.isObject = attrOf(item, "type") == "lv_obj"});
					for (const auto& described : item.children)
					{
						if (described.tag == "arg" || described.tag == "prop")
						{
							addAttributes(described, child.attributes);
						}
					}
				}
			}
			if (isWidget)
			{
				api.elements.push_back(std::move(element));
			}
		}
	} // namespace

	std::optional<LvglApi> readLvglApi(const std::string& dir, std::string& error)
	{
		std::vector<std::filesystem::path> files;
		std::error_code listError;
		for (const auto& entry : std::filesystem::directory_iterator(dir, listError))
		{
			if (entry.path().extension() == ".xml")
			{
				files.push_back(entry.path());
			}
		}
		if (listError)
		{
			error = fmt::format("{:s}: {:s}", dir, listError.message());
			return std::nullopt;
		}

		LvglApi api;
		for (const auto& file : files)
		{
			auto root = parseXmlFile(file.string(), error);
			if (!root)
			{
				return std::nullopt;
			}
			// A widget's description is <widget><api>…; globals.xml is the <api> itself
			const Element* description = root->tag == "api" ? &*root : root->child("api");
			if (description != nullptr)
			{
				readDescription(*description, file.stem().string(), api);
			}
		}
		std::sort(api.elements.begin(),
				  api.elements.end(),
				  [](const LvglElement& a, const LvglElement& b) { return a.name < b.name; });
		std::sort(
			api.enums.begin(), api.enums.end(), [](const LvglEnum& a, const LvglEnum& b) { return a.name < b.name; });
		return api;
	}
} // namespace UI::Xml::Api

/*
 * ApiMarkdown.cpp
 */

#include "ApiMarkdown.h"
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <iterator>

namespace UI::Xml::Api
{
	namespace
	{
		/** `text` made safe for a table cell: pipes escaped, line breaks as spaces */
		std::string cell(std::string_view text)
		{
			std::string result;
			for (const char c : text)
			{
				if (c == '|')
				{
					result += "\\|";
				}
				else if (c == '\n')
				{
					result += ' ';
				}
				else
				{
					result += c;
				}
			}
			return result;
		}

		/** `text` as inline code */
		std::string code(std::string_view text)
		{
			return fmt::format("`{:s}`", text);
		}

		/** A link to `path` from docs/ if it is relative, else the path as code */
		std::string fileLink(const std::string& path)
		{
			const std::filesystem::path p(path);
			if (p.is_absolute())
			{
				return code(path);
			}
			return fmt::format("[{:s}](../{:s})", p.filename().generic_string(), path);
		}

		/** Each name as inline code, joined with ", " */
		std::string codeList(const std::vector<std::string>& names)
		{
			std::vector<std::string> coded;
			for (const auto& name : names)
			{
				coded.push_back(code(name));
			}
			return fmt::format("{}", fmt::join(coded, ", "));
		}

		/**
		 * Write a table of `fields`, then a table of the rows of each list among them, and so on for nested lists
		 * @param path The lists these fields are rows of, outermost first; empty for top-level fields
		 */
		void writeFields(std::string& out, const std::vector<Field>& fields, const std::vector<std::string>& path = {})
		{
			if (fields.empty())
			{
				return;
			}
			if (!path.empty())
			{
				std::vector<std::string> coded;
				for (const auto& name : path)
				{
					coded.push_back(code(name));
				}
				fmt::format_to(std::back_inserter(out), "Each row of {}:\n\n", fmt::join(coded, " › "));
			}
			out += "| Name | Type | Description |\n| --- | --- | --- |\n";
			for (const auto& field : fields)
			{
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} |\n",
							   code(field.name),
							   field.type,
							   cell(field.description));
			}
			out += "\n";
			for (const auto& field : fields)
			{
				if (!field.rowFields.empty())
				{
					auto inner = path;
					inner.push_back(field.name);
					writeFields(out, field.rowFields, inner);
				}
			}
		}

		/** Write a table of `actions` */
		void writeActions(std::string& out, const std::vector<Action>& actions)
		{
			if (actions.empty())
			{
				return;
			}
			out += "| Action | Description |\n| --- | --- |\n";
			for (const auto& action : actions)
			{
				fmt::format_to(
					std::back_inserter(out), "| {:s} | {:s} |\n", code(action.name), cell(action.description));
			}
			out += "\n";
		}

		/** The global data section */
		void writeSources(std::string& out, const Reference& reference)
		{
			out += "## Global data\n\n"
				   "Printer data published by the data sources in [src/UI/Xml/Data](../src/UI/Xml/Data). Any view can "
				   "bind to these. A list is shown with `<ds_list list=\"…\">`; inside each row, the row's own names "
				   "are found first.\n\n";
			for (const auto& source : reference.sources)
			{
				fmt::format_to(std::back_inserter(out), "### {:s}\n\n", source.name);
				writeFields(out, source.fields);
			}
		}

		/** The global actions section */
		void writeGlobalActions(std::string& out, const Reference& reference)
		{
			out += "## Global actions\n\n"
				   "Actions any view can raise with `<action name=\"…\">`, whatever its presenter. A presenter's own "
				   "action of the same name is found first.\n\n";
			writeActions(out, reference.globalActions);
		}

		/** The presenters section. Presenters that provide exactly the same names share one entry. */
		void writePresenters(std::string& out, const Reference& reference)
		{
			out += "## Presenters\n\n"
				   "A component names its presenter with `<component presenter=\"…\">`. Its view can then bind to the "
				   "presenter's names and raise its actions, as well as the global ones.\n\n";

			// The body of each entry, and the presenters that have it, in the order first seen
			std::vector<std::pair<std::string, std::vector<const Presenter*>>> entries;
			for (const auto& presenter : reference.presenters)
			{
				std::string body;
				writeFields(body, presenter.fields);
				writeActions(body, presenter.actions);
				if (body.empty())
				{
					body = "Provides no names or actions.\n\n";
				}
				auto it = std::find_if(entries.begin(), entries.end(), [&](const auto& e) { return e.first == body; });
				if (it == entries.end())
				{
					entries.emplace_back(std::move(body), std::vector<const Presenter*>{&presenter});
				}
				else
				{
					it->second.push_back(&presenter);
				}
			}

			for (const auto& [body, presenters] : entries)
			{
				std::vector<std::string> names;
				std::vector<std::string> components;
				for (const Presenter* presenter : presenters)
				{
					names.push_back(presenter->name);
					components.insert(components.end(), presenter->components.begin(), presenter->components.end());
				}
				fmt::format_to(std::back_inserter(out), "### {:s}\n\n", fmt::join(names, ", "));
				if (!components.empty())
				{
					fmt::format_to(std::back_inserter(out), "Used by {:s}.\n\n", codeList(components));
				}
				out += body;
			}
		}

		/** The widgets section */
		void writeWidgets(std::string& out, const Reference& reference)
		{
			out += "## Widgets\n\n"
				   "DuetScreen's own elements, in addition to LVGL's `lv_*` widgets and the helix-xml elements (see "
				   "[helix-xml's README](../libraries/helix-xml/README.md)). An element registered as `parent-child` "
				   "is written inside a `parent`; `lv_obj-child` is written `<child>` inside any widget.\n\n";
			for (const auto& widget : reference.widgets)
			{
				fmt::format_to(std::back_inserter(out), "### {:s}\n\n{:s}\n\n", widget.name, widget.description);
				if (!widget.extends.empty())
				{
					fmt::format_to(
						std::back_inserter(out), "Also takes every {:s} attribute.\n\n", code(widget.extends));
				}
				if (!widget.example.empty())
				{
					fmt::format_to(std::back_inserter(out), "```xml\n{:s}\n```\n\n", widget.example);
				}
				if (!widget.attributes.empty())
				{
					out += "| Attribute | Description |\n| --- | --- |\n";
					for (const auto& attribute : widget.attributes)
					{
						fmt::format_to(std::back_inserter(out),
									   "| {:s} | {:s} |\n",
									   code(attribute.name),
									   cell(attribute.description));
					}
					out += "\n";
				}
			}
		}

		/** The components section */
		void writeComponents(std::string& out, const Reference& reference)
		{
			out += "## Components\n\n"
				   "The components of the built-in bundle. A user bundle replaces one by giving its own the same name; "
				   "a component is used as an element (`<fan_card/>`) or as a list's `item`.\n\n"
				   "| Component | Presenter | File |\n| --- | --- | --- |\n";
			for (const auto& component : reference.components)
			{
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} |\n",
							   code(component.name),
							   component.presenter.empty() ? "" : code(component.presenter),
							   fileLink(component.source));
			}
			out += "\n";
			if (!reference.nativeViews.empty())
			{
				fmt::format_to(std::back_inserter(out),
							   "C++ views that `<ds_view view=\"…\">` can show where there is no component of the "
							   "name: {:s}\n\n",
							   codeList(reference.nativeViews));
			}
		}

		/** The theme section */
		void writeTheme(std::string& out, const Reference& reference)
		{
			out += "## Theme\n\n"
				   "Themes are described in [XML_UI.md](XML_UI.md#themes). Everything here follows theme changes.\n\n";

			fmt::format_to(
				std::back_inserter(out),
				"### Styles\n\n"
				"Used with `<style name=\"…\"/>`. The definition is the one in the `{0:s}` theme, which every "
				"theme extends; other themes change some of them.\n\n"
				"| Style | Definition in `{0:s}` |\n| --- | --- |\n",
				reference.rootTheme);
			for (const auto& style : reference.styles)
			{
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} |\n",
							   code(style.name),
							   style.definition.empty() ? "*(not set)*" : cell(style.definition));
			}
			out += "\n";

			std::vector<std::string> colours;
			for (const auto& colour : reference.colours)
			{
				colours.push_back("#" + colour);
			}
			fmt::format_to(std::back_inserter(out),
						   "### Colours\n\n"
						   "Palette colours, used as attribute values, e.g. `style_bg_color=\"#primary\"`:\n\n"
						   "{:s}\n\n",
						   codeList(colours));

			fmt::format_to(std::back_inserter(out),
						   "### Consts\n\n"
						   "Used as attribute values, e.g. `style_radius=\"#radius\"`.\n\n"
						   "| Const | Value in `{:s}` |\n| --- | --- |\n",
						   reference.rootTheme);
			for (const auto& c : reference.consts)
			{
				fmt::format_to(std::back_inserter(out), "| {:s} | {:s} |\n", code("#" + c.name), cell(c.value));
			}
			out += "\n";

			fmt::format_to(std::back_inserter(out),
						   "### Fonts\n\n"
						   "Used as `style_text_font=\"…\"`: {:s}\n\n"
						   "### Images\n\n"
						   "Used as image sources, e.g. `style_bg_image_src=\"…\"`: {:s}\n\n",
						   codeList(reference.fonts),
						   codeList(reference.images));
		}
	} // namespace

	std::string toMarkdown(const Reference& reference)
	{
		std::string out;
		fmt::format_to(
			std::back_inserter(out),
			"# XML UI API reference\n\n"
			"<!-- Generated from the code by tests/src/test_cases/UI/test_xml_ui_api.cpp. Don't edit it: "
			"change the code, run the tests (which rewrite this file when it is out of date) and commit the "
			"result. -->\n\n"
			"Every name the firmware provides to XML, for UI API version {:s}. How to write XML with them is "
			"in [XML_UI.md](XML_UI.md). Translation keys, for `translation_tag`, are those in "
			"[en-GB.json](../assets/i18n/en-GB.json).\n\n"
			"- [Global data](#global-data)\n"
			"- [Global actions](#global-actions)\n"
			"- [Presenters](#presenters)\n"
			"- [Widgets](#widgets)\n"
			"- [Components](#components)\n"
			"- [Theme](#theme)\n\n",
			reference.apiVersion);
		writeSources(out, reference);
		writeGlobalActions(out, reference);
		writePresenters(out, reference);
		writeWidgets(out, reference);
		writeComponents(out, reference);
		writeTheme(out, reference);
		while (out.ends_with("\n\n"))
		{
			out.pop_back();
		}
		return out;
	}
} // namespace UI::Xml::Api

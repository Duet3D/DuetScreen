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

		/** The pictures, and how the document links to them */
		struct Pictures
		{
			const Images& images; // The pictures
			std::string root;	  // Path from the document to the reference images

			/** The pictures of the active theme */
			const ThemePictures& active() const
			{
				static const ThemePictures none;
				auto it = images.themes.find(images.theme);
				return it == images.themes.end() ? none : it->second;
			}

			/** Markdown for picture `file`, described by `alt`; empty if there is none */
			std::string image(std::string_view alt, const std::string& file) const
			{
				return file.empty() ? "" : fmt::format("![{:s}]({:s}{:s})", alt, root, file);
			}
		};

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
			fmt::format_to(std::back_inserter(out),
						   "An action runs when its widget raises its `trigger` (default `clicked`): {:s}\n\n",
						   codeList(reference.triggers));
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
		void writeWidgets(std::string& out, const Reference& reference, const Pictures& pictures)
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
				if (auto it = pictures.active().widgets.find(widget.name); it != pictures.active().widgets.end())
				{
					fmt::format_to(std::back_inserter(out), "{:s}\n\n", pictures.image("The example", it->second));
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
		void writeComponents(std::string& out, const Reference& reference, const Pictures& pictures)
		{
			out += "## Components\n\n"
				   "The components of the built-in bundle. A user bundle replaces one by giving its own the same name; "
				   "a component is used as an element (`<fan_card/>`) or as a list's `item`.\n\n"
				   "| Component | Presenter | Props | Used by | File |\n| --- | --- | --- | --- | --- |\n";
			for (const auto& component : reference.components)
			{
				std::vector<std::string> props;
				for (const auto& prop : component.props)
				{
					props.push_back(prop.name);
				}
				std::vector<std::string> users;
				for (const auto& use : component.uses)
				{
					if (std::find(users.begin(), users.end(), use.by) == users.end())
					{
						users.push_back(use.by);
					}
				}
				// Linked to the picture it is seen in, which is headed by the view whose picture it is
				std::string name = code(component.name);
				if (auto picture = pictures.images.components.find(component.name);
					picture != pictures.images.components.end())
				{
					for (const auto& [owner, file] : pictures.images.views)
					{
						if (file == picture->second)
						{
							name = fmt::format("[{:s}](#{:s})", name, owner);
						}
					}
				}
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} | {:s} | {:s} |\n",
							   name,
							   component.presenter.empty() ? "" : code(component.presenter),
							   codeList(props),
							   codeList(users),
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

			if (!pictures.images.views.empty())
			{
				fmt::format_to(
					std::back_inserter(out),
					"### Views\n\n"
					"The components shown as views of their own, drawn with sample printer data in the `{:s}` "
					"theme. A view that shows in another's picture (the status bar in the home screen's) is "
					"pictured only there.\n\n",
					pictures.images.theme);
				for (const auto& [owner, file] : pictures.images.views)
				{
					std::vector<std::string> alsoShown;
					for (const auto& [component, seenIn] : pictures.images.components)
					{
						if (seenIn == file && component != owner)
						{
							alsoShown.push_back(component);
						}
					}
					fmt::format_to(std::back_inserter(out), "#### {:s}\n\n", owner);
					if (!alsoShown.empty())
					{
						fmt::format_to(std::back_inserter(out), "Also shows {:s}.\n\n", codeList(alsoShown));
					}
					fmt::format_to(std::back_inserter(out), "{:s}\n\n", pictures.image(owner, file));
				}
			}
		}

		/** The theme section */
		void writeTheme(std::string& out, const Reference& reference, const Pictures& pictures)
		{
			out += "## Theme\n\n"
				   "Themes are described in [XML_UI.md](XML_UI.md#themes), and every built-in theme, with what it "
				   "changes and how it looks, in [XML_UI_THEMES.md](XML_UI_THEMES.md). Everything here follows theme "
				   "changes.\n\n";

			fmt::format_to(
				std::back_inserter(out),
				"### Styles\n\n"
				"Used with `<style name=\"…\"/>`. The definition is the one in the `{0:s}` theme, which every "
				"theme extends; other themes change some of them.\n\n"
				"The swatch is a box with some text, with the style added, in the `{1:s}` theme; a style that "
				"changes nothing on its own (a state's, or a part's) has none.\n\n"
				"| Style | Swatch | Definition in `{0:s}` |\n| --- | --- | --- |\n",
				reference.rootTheme,
				pictures.images.theme);
			for (const auto& style : reference.styles)
			{
				const auto& swatches = pictures.active().styles;
				const auto swatch = swatches.find(style.name);
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} |\n",
							   code(style.name),
							   swatch == swatches.end() ? "" : pictures.image(style.name, swatch->second),
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
			if (!pictures.active().colours.empty())
			{
				fmt::format_to(
					std::back_inserter(out), "{:s}\n\n", pictures.image("The palette", pictures.active().colours));
			}

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
						   "Used as `style_text_font=\"…\"`: {:s}\n\n",
						   codeList(reference.fonts));
			if (!pictures.active().fonts.empty())
			{
				fmt::format_to(
					std::back_inserter(out), "{:s}\n\n", pictures.image("The fonts", pictures.active().fonts));
			}
			fmt::format_to(std::back_inserter(out),
						   "### Images\n\n"
						   "Used as image sources, e.g. `style_bg_image_src=\"…\"`: {:s}\n\n",
						   codeList(reference.images));
		}
	} // namespace

	std::string toMarkdown(const Reference& reference, const Images& images, std::string_view imageRoot)
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
		const Pictures pictures{.images = images, .root = std::string(imageRoot)};
		writeWidgets(out, reference, pictures);
		writeComponents(out, reference, pictures);
		writeTheme(out, reference, pictures);
		while (out.ends_with("\n\n"))
		{
			out.pop_back();
		}
		return out;
	}

	std::string toThemesMarkdown(const Reference& reference, const Images& images, std::string_view imageRoot)
	{
		const Pictures pictures{.images = images, .root = std::string(imageRoot)};
		std::string out =
			"# Built-in themes\n\n"
			"<!-- Generated from the code by tests/src/test_cases/UI/test_xml_ui_api.cpp, like XML_UI_API.md. Don't "
			"edit it. -->\n\n"
			"The themes the firmware comes with: what each one's file sets, and how it draws. A theme extends another "
			"and changes only what makes it different (see [XML_UI.md](XML_UI.md#themes)); the names it uses are in "
			"[XML_UI_API.md](XML_UI_API.md#theme). A style's swatch is a box with some text, with the style added; "
			"a style that changes nothing on its own (a state's, or a part's) has none.\n\n";
		for (const auto& theme : reference.themes)
		{
			fmt::format_to(std::back_inserter(out), "- [{0:s}](#{0:s})\n", theme.name);
		}
		out += "- [Styles](#styles), in every theme\n\n";

		for (const auto& theme : reference.themes)
		{
			fmt::format_to(std::back_inserter(out), "## {:s}\n\n", theme.name);
			std::string about =
				theme.extends.empty() ? "The root theme" : fmt::format("Extends {:s}", code(theme.extends));
			if (theme.abstract)
			{
				about += "; only a base for other themes, so not offered in the settings";
			}
			fmt::format_to(std::back_inserter(out), "{:s}. Its file is {:s}.\n\n", about, fileLink(theme.source));

			std::vector<std::string> palette;
			for (const auto& [attr, value] : theme.palette)
			{
				palette.push_back(fmt::format("{:s}={:s}", attr, value));
			}
			if (!palette.empty())
			{
				fmt::format_to(std::back_inserter(out), "Palette: {:s}\n\n", codeList(palette));
			}
			if (!theme.colours.empty())
			{
				fmt::format_to(std::back_inserter(out), "Colour adjustments: {:s}\n\n", codeList(theme.colours));
			}
			if (!theme.fonts.empty())
			{
				fmt::format_to(std::back_inserter(out), "Fonts: {:s}\n\n", fmt::join(theme.fonts, "; "));
			}
			if (!theme.consts.empty())
			{
				std::vector<std::string> consts;
				for (const auto& c : theme.consts)
				{
					consts.push_back(fmt::format("#{:s}={:s}", c.name, c.value));
				}
				fmt::format_to(std::back_inserter(out), "Consts: {:s}\n\n", codeList(consts));
			}

			auto drawn = images.themes.find(theme.name);
			if (drawn != images.themes.end())
			{
				const ThemePictures& p = drawn->second;
				for (const auto& [alt, file] : {std::pair<const char*, const std::string&>{"The palette", p.colours},
												{"The fonts", p.fonts},
												{"The home screen", p.home},
												{"LVGL's widgets", p.lvglWidgets}})
				{
					if (!file.empty())
					{
						fmt::format_to(std::back_inserter(out), "{:s}\n\n", pictures.image(alt, file));
					}
				}
				if (!p.widgets.empty())
				{
					out += "| Widget | Its example |\n| --- | --- |\n";
					for (const auto& [widget, file] : p.widgets)
					{
						fmt::format_to(std::back_inserter(out),
									   "| [{:s}](XML_UI_API.md#{:s}) | {:s} |\n",
									   code(widget),
									   widget,
									   pictures.image(widget, file));
					}
					out += "\n";
				}
			}
		}

		// Every style, with a swatch in each theme that is drawn
		std::vector<const Theme*> drawn;
		for (const auto& theme : reference.themes)
		{
			if (images.themes.contains(theme.name))
			{
				drawn.push_back(&theme);
			}
		}
		out += "## Styles\n\n"
			   "Each style's swatch in each theme. What `base` sets in each is in "
			   "[XML_UI_API.md](XML_UI_API.md#styles); what a theme changes is in its file.\n\n"
			   "| Style |";
		for (const Theme* theme : drawn)
		{
			fmt::format_to(std::back_inserter(out), " {:s} |", theme->name);
		}
		out += "\n| --- |";
		for (size_t i = 0; i < drawn.size(); i++)
		{
			out += " --- |";
		}
		out += "\n";
		for (const auto& style : reference.styles)
		{
			fmt::format_to(std::back_inserter(out), "| {:s} |", code(style.name));
			for (const Theme* theme : drawn)
			{
				const auto& swatches = images.themes.at(theme->name).styles;
				auto swatch = swatches.find(style.name);
				fmt::format_to(std::back_inserter(out),
							   " {:s} |",
							   swatch == swatches.end() ? "" : pictures.image(style.name, swatch->second));
			}
			out += "\n";
		}

		while (out.ends_with("\n\n"))
		{
			out.pop_back();
		}
		return out;
	}
} // namespace UI::Xml::Api

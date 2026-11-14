/*
 * ApiMarkdown.cpp
 */

#include "ApiMarkdown.h"
#include "ObjectModel/Tree/Schema.h"
#include "i18n/i18n.h"
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
			const bool files =
				std::any_of(actions.begin(), actions.end(), [](const Action& action) { return !action.file.empty(); });
			out += files ? "| Action | Description | Added in |\n| --- | --- | --- |\n"
						 : "| Action | Description |\n| --- | --- |\n";
			for (const auto& action : actions)
			{
				fmt::format_to(std::back_inserter(out), "| {:s} | {:s} |", code(action.name), cell(action.description));
				if (files)
				{
					const std::string name = std::filesystem::path(action.file).filename().string();
					fmt::format_to(std::back_inserter(out), " [{:s}](../{:s}) |", name, action.file);
				}
				out += "\n";
			}
			out += "\n";
		}

		/** The object model section */
		void writeObjectModel(std::string& out, const Reference& reference)
		{
			out +=
				"## Object model\n\n"
				"Every field of the printer's object model has a name, written as RepRapFirmware writes its path "
				"after `om.`: `om.heat.heaters[1].current`, `om.move.axes[0].userPosition`, `om.state.status`. The "
				"names need no code: a field a newer firmware adds can be used as soon as the printer sends it. "
				"Every field is listed in [XML_UI_OBJECT_MODEL.md](XML_UI_OBJECT_MODEL.md); see also "
				"[ObjectModel/Tree](../src/ObjectModel/Tree) and [src/UI/Xml/Om](../src/UI/Xml/Om).\n\n"
				"- A name binds before the printer has sent the field, and follows it: while the field isn't there "
				"(or is null) its subject holds empty text, or 0.\n"
				"- An array is a list: `<ds_list list=\"om.heat.heaters\" item=\"…\">` makes a row per item, nulls "
				"included, so a row's `index` is the item's index (the heater number). Inside a row, the item's "
				"fields have names relative to it, `current` or `model.pid.p`, found before any other name, and its "
				"arrays are lists: `<ds_list list=\"heaters\">` in a row of `om.tools`. A name the item doesn't have "
				"is looked up outside it, so in a row of a tool's `heaters`, `active[{index}]` is the tool's.\n"
				"- An index can be another subject's value, written `{name}`, which joins one part of the object "
				"model to another: `om.heat.heaters[{value}].current` in a row of a tool's heaters, or "
				"`om.tools[{om.state.currentTool}].name`. The name follows when the index changes; nothing is "
				"rebuilt.\n"
				"- `<ds_scope path=\"om.…\" item=\"…\">` builds a component whose names are relative to one "
				"element, as in a list row.\n"
				"- The name sets what the subject holds: the value as text, or after `|` what a converter makes of "
				"it: `om.fans[0].actualValue|percent`. A converter's argument follows `:`, as in `|fixed:1`.\n\n";
			out += "Each row of an object model list:\n\n| Name | Type | Description |\n| --- | --- | --- |\n";
			for (const auto& field : reference.omRowFields)
			{
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} |\n",
							   code(field.name),
							   field.type,
							   cell(field.description));
			}
			out += "| *a field of the item* | text | e.g. `current`; converters apply, as in `current\\|fixed:1` |\n\n";
			out += "### Converters\n\n| Converter | Gives | Argument | Description |\n| --- | --- | --- | --- |\n";
			for (const auto& converter : reference.omConverters)
			{
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} | {:s} |\n",
							   converter.name.empty() ? "*none*" : cell(code("|" + std::string(converter.name))),
							   converter.type,
							   converter.argument.empty() ? "" : cell(converter.argument),
							   cell(converter.description));
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
				if (source.fields.empty())
				{
					continue; // e.g. the object model's, whose names are given on demand
				}
				fmt::format_to(std::back_inserter(out), "### {:s}\n\n", source.name);
				writeFields(out, source.fields);
			}
		}

		/** The global actions section */
		void writeGlobalActions(std::string& out, const Reference& reference)
		{
			out += "## Global actions\n\n"
				   "Actions any view can raise with `<action name=\"actions.<group>.<action>\">`, whatever its "
				   "presenter. A group's actions are added in `<Group>Actions.cpp`. A presenter's own actions are "
				   "written `presenter.<action>`.\n\n";
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

		/** Text shown for `tag` translated, or `fallback` if there is no tag */
		std::string shown(const std::string& tag, const std::string& fallback)
		{
			return tag.empty() ? fallback : std::string(_(tag.c_str()));
		}

		/** What values a setting takes, e.g. "int, 10 to 600 s" */
		std::string settingKind(const SettingDef& setting)
		{
			switch (setting.kind)
			{
			case SettingKind::Int:
				return fmt::format("int, {:d} to {:d}{:s}{:s}",
								   setting.min,
								   setting.max,
								   setting.unit.empty() ? "" : " ",
								   setting.unit);
			case SettingKind::Bool:
				return "on or off";
			case SettingKind::Text:
				return fmt::format("text, up to {:d} bytes", setting.maxLength);
			case SettingKind::Choice:
			{
				std::vector<std::string> values;
				for (const auto& option : setting.options)
				{
					values.push_back(code(option.value));
				}
				return fmt::format("one of {:s}", fmt::join(values, ", "));
			}
			case SettingKind::Row:
				return fmt::format("a row of {:s}, by its {:s}, shown by its {:s}",
								   code(setting.list),
								   code(setting.key),
								   code(setting.label));
			}
			return "";
		}

		/** A size in pixels, e.g. "300×140 px" */
		std::string pixels(const Size& size)
		{
			return fmt::format("{:d}×{:d} px", size.width, size.height);
		}

		/** The layouts section: each slot's default layout, then every widget a user can place */
		void writeLayouts(std::string& out, const Reference& reference, const Pictures& pictures)
		{
			out += "## Layouts\n\n"
				   "What the user places on the screen, and where; see Layouts in [XML_UI.md](XML_UI.md). A "
				   "`<ds_layout slot=\"…\">` shows its slot's layout, a file in a bundle's `layouts/`. A free layout "
				   "places widgets in rectangles of pixels; a strip shows them one after another. Locked items can be "
				   "moved "
				   "but not removed.\n\n"
				   "### Slots\n\n"
				   "The built-in slots and the layout each shows until the user changes it.\n\n"
				   "| Slot | Kind | Items (id: widget) |\n| --- | --- | --- |\n";
			for (const auto& layout : reference.layouts)
			{
				std::vector<std::string> items;
				for (const auto& item : layout.items)
				{
					std::string text = fmt::format("{:s}: {:s}", code(item.id), code(item.widget));
					if (layout.kind == LayoutKind::Free)
					{
						text += fmt::format(" at {:d},{:d}, {:d}×{:d}", item.x, item.y, item.w, item.h);
					}
					if (item.locked)
					{
						text += ", locked";
					}
					items.push_back(std::move(text));
				}
				fmt::format_to(std::back_inserter(out),
							   "| {:s} | {:s} | {:s} |\n",
							   code(layout.slot),
							   layout.kind == LayoutKind::Free ? fmt::format("free, {:s}", pixels(layout.area()))
															   : std::string("strip"),
							   items.empty() ? std::string("none") : fmt::format("{:s}", fmt::join(items, "; ")));
			}

			out += "\n### Placed widgets' names\n\n"
				   "A placed widget's component can use these as well as its presenter's names and the globals (they "
				   "are found after its presenter's), to fit what it shows to the room it has, e.g. "
				   "`<bind_flag_if cond=\"widget.width_class eq 0\" flag=\"hidden\"/>` hides a part when the "
				   "widget is narrow. The breakpoints are the theme's consts `widget_regular_min` and "
				   "`widget_expanded_min`. See [PlacementScope.h](../src/UI/Xml/Layout/PlacementScope.h) for when to "
				   "use them.\n\n";
			writeFields(out, reference.placedFields);

			out +=
				"### Placeable widgets\n\n"
				"Each widget is a component (with its props) and a widget file beside it, which says where it can be "
				"placed, its sizes in pixels, and its settings: props the user chooses on the screen. Each is "
				"pictured as placed, at its default size in its first slot, with its default settings; a free layout's "
				"widget is pictured at its smallest size too.\n\n";
			for (const auto& widget : reference.placeable)
			{
				fmt::format_to(std::back_inserter(out),
							   "#### {:s}\n\n**{:s}**: {:s}\n\n"
							   "In {:s}; placed in {:s}; component {:s} ({:s}).",
							   widget.id,
							   shown(widget.titleTag, widget.title),
							   shown(widget.descriptionTag, widget.description),
							   shown(widget.categoryTag, widget.category.empty() ? "no category" : widget.category),
							   codeList(widget.slots),
							   code(widget.component),
							   fileLink(widget.source.generic_string()));
				if (std::any_of(reference.layouts.begin(),
								reference.layouts.end(),
								[&](const Layout& layout)
								{ return layout.kind == LayoutKind::Free && widget.fits(layout.slot); }))
				{
					const bool unbounded = widget.maxSize == UNBOUNDED;
					fmt::format_to(
						std::back_inserter(out),
						" In a free layout it is {:s}, and {:s}.",
						pixels(widget.defaultSize),
						unbounded ? fmt::format("at least {:s}", pixels(widget.minSize))
								  : fmt::format("from {:s} to {:s}", pixels(widget.minSize), pixels(widget.maxSize)));
				}
				out += "\n\n";
				if (auto it = pictures.images.placeable.find(widget.id); it != pictures.images.placeable.end())
				{
					fmt::format_to(std::back_inserter(out), "{:s}\n\n", pictures.image(widget.id, it->second));
				}
				if (auto it = pictures.images.placeableSmallest.find(widget.id);
					it != pictures.images.placeableSmallest.end())
				{
					fmt::format_to(std::back_inserter(out),
								   "At its smallest, {:s}:\n\n{:s}\n\n",
								   pixels(widget.minSize),
								   pictures.image(widget.id + " at its smallest", it->second));
				}
				if (!widget.settings.empty())
				{
					out += "| Setting | Shown as | Takes | Default |\n| --- | --- | --- | --- |\n";
					for (const auto& setting : widget.settings)
					{
						fmt::format_to(std::back_inserter(out),
									   "| {:s} | {:s} | {:s} | {:s} |\n",
									   code(setting.name),
									   cell(shown(setting.titleTag, setting.title)),
									   cell(settingKind(setting)),
									   code(SettingDef::toProp(setting.fallback)));
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
			"- [Object model](#object-model)\n"
			"- [Global data](#global-data)\n"
			"- [Global actions](#global-actions)\n"
			"- [Presenters](#presenters)\n"
			"- [Widgets](#widgets)\n"
			"- [Layouts](#layouts)\n"
			"- [Components](#components)\n"
			"- [Theme](#theme)\n\n",
			reference.apiVersion);
		writeObjectModel(out, reference);
		writeSources(out, reference);
		writeGlobalActions(out, reference);
		writePresenters(out, reference);
		const Pictures pictures{.images = images, .root = std::string(imageRoot)};
		writeWidgets(out, reference, pictures);
		writeLayouts(out, reference, pictures);
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
	std::string toObjectModelMarkdown()
	{
		const auto& schema = OM::Schema::get();
		std::string out = fmt::format(
			"# Object model fields\n\n"
			"<!-- Generated from assets/object_model/schema.json (scripts/generate_om_schema.py) by "
			"tests/src/test_cases/UI/test_xml_ui_api.cpp. Don't edit it here. -->\n\n"
			"Every field the printer's object model can have, from {:s}, for the `om.*` names XML binds to (see "
			"[Object model](XML_UI_API.md#object-model)). `[]` stands for an index, e.g. `heat.heaters[1].current`, "
			"and `{{}}` for a name the printer chooses, e.g. a global variable's. A field this doesn't list (one a "
			"newer "
			"firmware added) can still be used once the printer sends it.\n\n"
			"Types: a number, string (text) or boolean (shown as true or false; `|int` gives 1 or 0), an object of the "
			"named type, an enum (text, one of its values), or `T[]`, an array of T. A field can also be null, which "
			"shows as empty text, or 0.\n",
			schema.source().empty() ? "no schema" : schema.source());

		std::string section;
		schema.forEachField(
			[&](const std::string& path, const std::string& type)
			{
				// A table per top-level field
				const std::string top = path.substr(0, path.find_first_of(".["));
				if (top != section)
				{
					section = top;
					fmt::format_to(std::back_inserter(out), "\n## {:s}\n\n| Field | Type |\n| --- | --- |\n", top);
				}
				std::string described = type;
				const std::string base = type.substr(0, type.find('['));
				if (const auto* values = schema.enumValues(base))
				{
					described += fmt::format(": {}", fmt::join(*values, ", "));
				}
				fmt::format_to(std::back_inserter(out), "| {:s} | {:s} |\n", code(path), cell(described));
			});
		return out;
	}
} // namespace UI::Xml::Api

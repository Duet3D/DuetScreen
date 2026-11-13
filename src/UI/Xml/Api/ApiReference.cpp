/*
 * ApiReference.cpp
 */

#include "ApiReference.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/Binding/Fields.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/Theme/ThemeStyles.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "XmlTree.h"
#include "helix_xml.h"
#include "i18n/i18n.h"
#include "src/xml/lv_xml_widget.h"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <map>
#include <set>

namespace UI::Xml::Api
{
	namespace
	{
		/** Collects the fields it visits, and the fields of each list's rows */
		class FieldCollector : public FieldVisitor
		{
		  public:
			/** Collect into `fields` */
			explicit FieldCollector(std::vector<Field>& fields)
				: m_fields(fields)
			{
			}

			/** Add the subject */
			void field(std::string_view name, Subject& subject, std::string_view description) override
			{
				m_fields.push_back({.name = std::string(name),
									.type = std::string(subject.typeName()),
									.description = std::string(description)});
			}

			/** Add the list, with its row fields */
			void list(std::string_view name, ListBase& list, std::string_view description) override
			{
				Field field{.name = std::string(name),
							.type = std::string(list.typeName()),
							.description = std::string(description)};
				FieldCollector rows(field.rowFields);
				list.rowFields(rows);
				m_fields.push_back(std::move(field));
			}

		  private:
			std::vector<Field>& m_fields; // Where fields are collected
		};

		/** The actions in `set` */
		std::vector<Action> actionsOf(const ActionHandlerSet& set)
		{
			std::vector<Action> actions;
			set.forEach([&](std::string_view name, std::string_view description)
						{ actions.push_back({.name = std::string(name), .description = std::string(description)}); });
			return actions;
		}

		/** The data sources and what they publish */
		std::vector<Source> sources()
		{
			std::vector<Source> result;
			for (const auto& source : DataSources::all())
			{
				Source& entry = result.emplace_back(Source{.name = std::string(source->getName()), .fields = {}});
				FieldCollector collector(entry.fields);
				source->fields(collector);
			}
			return result;
		}

		/** Every registered presenter, with the loaded components that name it */
		std::vector<Presenter> presenters()
		{
			std::vector<Presenter> result;
			for (const auto& name : Presenters::names())
			{
				auto presenter = Presenters::create(name);
				if (presenter == nullptr)
				{
					continue;
				}
				Presenter& entry = result.emplace_back(Presenter{
					.name = name, .fields = {}, .actions = actionsOf(presenter->getActions()), .components = {}});
				FieldCollector collector(entry.fields);
				presenter->fields(collector);
				for (const auto& [componentName, info] : getComponents())
				{
					if (info.presenter == name)
					{
						entry.components.push_back(componentName);
					}
				}
			}
			return result;
		}

		/** Every loaded component */
		/** Add to `uses` each use, by `user`, of the components in `names`, in `element` and its descendants */
		void collectUses(const Element& element,
						 const std::string& user,
						 const std::set<std::string, std::less<>>& names,
						 std::map<std::string, std::vector<Use>>& uses)
		{
			const auto use = [&](const std::string* name, Use::As as, const std::string* presenter = nullptr)
			{
				if (name && names.contains(*name))
				{
					uses[*name].push_back({.by = user, .as = as, .presenter = presenter ? *presenter : ""});
				}
			};
			if (element.tag == "view")
			{
				use(element.attr("extends"), Use::As::Base);
			}
			else if (element.tag == "ds_view")
			{
				use(element.attr("view"), Use::As::View, element.attr("presenter"));
			}
			else
			{
				use(&element.tag, Use::As::Element);
			}
			use(element.attr("item"), Use::As::Item);
			for (const auto& child : element.children)
			{
				collectUses(child, user, names, uses);
			}
		}

		/** Every loaded component, with its props and where it is used */
		std::vector<Component> components()
		{
			std::vector<Component> result;
			std::map<std::string, Element> roots;
			for (const auto& [name, info] : getComponents())
			{
				Component& component = result.emplace_back(Component{.name = name,
																	 .presenter = info.presenter,
																	 .bundle = info.bundle,
																	 .source = info.source.generic_string()});
				// The file loaded, so it parses
				std::string error;
				auto root = parseXmlFile(component.source, error);
				if (!root)
				{
					continue;
				}
				const Element* view = root->child("view");
				const std::string* extends = view ? view->attr("extends") : nullptr;
				component.extends = extends ? *extends : "lv_obj";
				// Its props are the `<prop>`s of its `<api>`
				const Element* api = root->child("api");
				for (const auto& prop : api ? api->children : std::vector<Element>{})
				{
					const std::string* propName = prop.attr("name");
					if (prop.tag != "prop" || propName == nullptr)
					{
						continue;
					}
					const std::string* type = prop.attr("type");
					const std::string* fallback = prop.attr("default");
					component.props.push_back(
						{.name = *propName, .type = type ? *type : "", .defaultValue = fallback ? *fallback : ""});
				}
				roots.emplace(name, std::move(*root));
			}

			std::set<std::string, std::less<>> names;
			for (const auto& component : result)
			{
				names.insert(component.name);
			}
			std::map<std::string, std::vector<Use>> uses;
			for (const auto& [name, root] : roots)
			{
				collectUses(root, name, names, uses);
			}
			for (auto& component : result)
			{
				component.uses = std::move(uses[component.name]);
			}
			return result;
		}

		/** What `file` sets in the style called `name`, as written there: copies, merges, then properties */
		std::string definitionOf(const Themes::ThemeFile& file, std::string_view name)
		{
			std::vector<std::string> parts;
			for (const auto& edit : file.styles)
			{
				if (edit.name != name)
				{
					continue;
				}
				if (!edit.copy.empty())
				{
					parts.push_back(fmt::format("copy={:s}", edit.copy));
				}
				for (const auto& merged : edit.merge)
				{
					parts.push_back(fmt::format("merge={:s}", merged));
				}
				for (const auto& [prop, value] : edit.props)
				{
					parts.push_back(fmt::format("{:s}={:s}", prop, value));
				}
			}
			return fmt::format("{}", fmt::join(parts, " "));
		}

		/** `change` as written in a `<color>` attribute: "+0.1", "*0.5" or "0.3" */
		std::string describeChange(const Themes::ColorAdjustment::Change& change)
		{
			using Op = Themes::ColorAdjustment::Change::Op;
			const char* prefix = change.op == Op::Add		 ? (change.amount < 0 ? "" : "+")
								 : change.op == Op::Multiply ? "*"
															 : "";
			return fmt::format("{:s}{:g}", prefix, change.amount);
		}

		/** `adjustment` as its `<color>` element's attributes, e.g. "primary lightness=+0.1 when=dark" */
		std::string describeAdjustment(const Themes::ColorAdjustment& adjustment)
		{
			using When = Themes::ColorAdjustment::When;
			std::string text = adjustment.color;
			if (!adjustment.from.empty())
			{
				text += fmt::format(" from={:s}", adjustment.from);
			}
			if (adjustment.lightness)
			{
				text += fmt::format(" lightness={:s}", describeChange(*adjustment.lightness));
			}
			if (adjustment.chroma)
			{
				text += fmt::format(" chroma={:s}", describeChange(*adjustment.chroma));
			}
			if (adjustment.when != When::Always)
			{
				text += adjustment.when == When::Dark ? " when=dark" : " when=light";
			}
			return text;
		}

		/** `font` as text, e.g. "18 px, weight 700, bold"; what it leaves to its parent theme is left out */
		std::string describeFont(const Themes::FontSpec& font)
		{
			std::vector<std::string> parts;
			if (font.size)
			{
				parts.push_back(fmt::format("{} px", *font.size));
			}
			if (font.weight)
			{
				parts.push_back(*font.weight == 0 ? "the face's own weight" : fmt::format("weight {}", *font.weight));
			}
			if (font.style)
			{
				parts.push_back(*font.style);
			}
			return fmt::format("{}", fmt::join(parts, ", "));
		}

		/** Every loaded theme as its file sets it, the root of the active theme first */
		std::vector<Theme> themes(const std::string& root)
		{
			std::vector<std::string> names;
			// The loaded files are found through the chains of the themes users can choose, and the root
			for (const auto name : Themes::getThemeNames())
			{
				for (const Themes::ThemeFile* file : Themes::getThemeChain(name))
				{
					if (std::find(names.begin(), names.end(), file->name) == names.end())
					{
						names.push_back(file->name);
					}
				}
			}
			std::sort(names.begin(),
					  names.end(),
					  [&](const std::string& a, const std::string& b)
					  { return (a == root) != (b == root) ? a == root : a < b; });

			std::vector<Theme> result;
			for (const auto& name : names)
			{
				const auto chain = Themes::getThemeChain(name);
				if (chain.empty())
				{
					continue;
				}
				const Themes::ThemeFile& file = *chain.back();
				Theme& theme = result.emplace_back(Theme{.name = file.name,
														 .extends = file.extends,
														 .abstract = file.abstract,
														 .source = file.source.generic_string(),
														 .palette = {file.palette.begin(), file.palette.end()},
														 .colours = {},
														 .fonts = {},
														 .consts = {}});
				for (const auto& adjustment : file.colors)
				{
					theme.colours.push_back(describeAdjustment(adjustment));
				}
				for (const auto& [fontName, font] : file.fonts)
				{
					theme.fonts.push_back(fmt::format("{:s}: {:s}", fontName, describeFont(font)));
				}
				for (const auto& [constName, value] : file.consts)
				{
					theme.consts.push_back({.name = constName, .value = value});
				}
			}
			return result;
		}

		/** Fill in the styles, consts and colours, described from the root of the active theme */
		void describeTheme(Reference& reference)
		{
			const Themes::Theme* theme = Themes::getCurrentTheme();
			const auto chain =
				theme ? Themes::getThemeChain(theme->getName()) : std::vector<const Themes::ThemeFile*>{};
			const Themes::ThemeFile* root = chain.empty() ? nullptr : chain.front();
			reference.rootTheme = root ? root->name : "";

			Themes::forEachStyle(
				[&](std::string_view name, Themes::Style&)
				{
					reference.styles.push_back(
						{.name = std::string(name), .definition = root ? definitionOf(*root, name) : ""});
				});

			if (root)
			{
				for (const auto& [name, value] : root->consts)
				{
					reference.consts.push_back({.name = name, .value = value});
				}
			}

			Themes::ThemeColors colors{};
			Themes::forEachColor(colors, [&](std::string_view name, Color&) { reference.colours.emplace_back(name); });
		}

		/**
		 * The widgets and elements the XML engine itself provides (lv_xml_init()'s, not DuetScreen's), as registered:
		 * `lv_label`, or `lv_obj-bind_flag_if_eq` for a child element of a widget. In name order.
		 */
		std::vector<std::string> lvglElements()
		{
			std::vector<std::string> names;
			lv_xml_widget_foreach(
				[](const lv_widget_processor_t* processor, void* userData)
				{
					if (processor->builtin)
					{
						static_cast<std::vector<std::string>*>(userData)->emplace_back(processor->name);
					}
				},
				&names);
			std::sort(names.begin(), names.end());
			return names;
		}
	} // namespace

	bool Component::isView() const
	{
		return uses.empty() ||
			   std::any_of(uses.begin(), uses.end(), [](const Use& use) { return use.as == Use::As::View; });
	}

	Reference build()
	{
		Reference reference;
		reference.apiVersion = fmt::format("{}.{}", UI_API_VERSION_MAJOR, UI_API_VERSION_MINOR);
		reference.sources = sources();
		reference.triggers = Actions::triggers();
		reference.globalActions = actionsOf(Actions::globals());
		reference.presenters = presenters();
		for (const WidgetSpec* spec : Widgets::specs())
		{
			reference.widgets.push_back(*spec);
		}
		reference.lvglElements = lvglElements();
		reference.components = components();
		describeTheme(reference);
		reference.themes = themes(reference.rootTheme);
		for (const auto name : ThemeStyles::fontNames())
		{
			reference.fonts.emplace_back(name);
		}
		for (const auto name : ThemeStyles::imageNames())
		{
			reference.images.emplace_back(name);
		}
		i18n::forEachTranslation([&](std::string_view key, std::string_view)
								 { reference.translationKeys.emplace_back(key); });
		std::sort(reference.translationKeys.begin(), reference.translationKeys.end());
		return reference;
	}
} // namespace UI::Xml::Api

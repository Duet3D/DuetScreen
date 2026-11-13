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
#include "UI/Xml/View/NativeViews.h"
#include "UI/Xml/View/XmlPresenter.h"
#include "XmlTree.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>

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
		std::vector<Component> components()
		{
			std::vector<Component> result;
			for (const auto& [name, info] : getComponents())
			{
				Component& component = result.emplace_back(Component{.name = name,
																	 .presenter = info.presenter,
																	 .bundle = info.bundle,
																	 .source = info.source.generic_string()});
				// The file loaded, so it parses; its props are the `<prop>`s of its `<api>`
				std::string error;
				const auto root = parseXmlFile(component.source, error);
				const Element* api = root ? root->child("api") : nullptr;
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
	} // namespace

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
		reference.components = components();
		reference.nativeViews = NativeViews::names();
		describeTheme(reference);
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

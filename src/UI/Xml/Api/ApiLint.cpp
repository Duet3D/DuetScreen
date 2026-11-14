/*
 * ApiLint.cpp
 */

#include "ApiLint.h"
#include "ObjectModel/Tree/Schema.h"
#include "UI/Xml/Om/Converters.h"
#include "UI/Xml/Om/OmName.h"
#include "UI/Xml/Om/OmPath.h"
#include "UI/Xml/Runtime/Translations.h"
#include "UI/Xml/View/Actions.h"
#include "XmlTree.h"
#include "helix_xml.h"
#include "src/xml/lv_xml_widget.h"
#include <algorithm>
#include <cctype>
#include <deque>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <sstream>

namespace UI::Xml::Api
{
	namespace
	{
		using NameSet = std::set<std::string, std::less<>>;

		/** A component, as read from its file */
		struct ComponentDef
		{
			std::string name;							// Component name
			std::string presenter;						// Presenter it names, or empty
			std::string file;							// File it was loaded from
			Element root;								// Its `<component>` element
			std::map<std::string, std::string> props{}; // Declared props, with their defaults ("" for none)
			NameSet styles{};							// Styles it defines in `<styles>`
			NameSet subjects{};							// Subjects it declares in `<subjects>`
			NameSet consts{};							// Consts it defines in `<consts>`
		};

		/** Where names are looked up */
		struct Scope
		{
			std::vector<const std::vector<Field>*> fields{}; // Innermost first: list rows, then the view's presenter
			std::vector<const Presenter*> handlers{};		 // Presenters actions go to, innermost view first
			std::string where{};							 // Path of components to here, for messages
			std::vector<OM::Path> omItems{}; // The object model items (rows of `om.` lists, <ds_scope>s) this is in,
											 // innermost first, any index as 0. Their fields have names relative to
											 // them, looked up innermost first.
		};

		/** One use of a component: where it is, and its props */
		struct Context
		{
			const ComponentDef* def;						 // The component
			const std::map<std::string, std::string>* props; // Prop values given where it is used
			Scope scope;									 // Where its names are looked up
		};

		/** Word operators and literals of the expression language, which aren't subject names */
		const NameSet EXPRESSION_WORDS = {"or", "and", "not", "eq", "ne", "lt", "le", "gt", "ge", "true", "false"};

		/** Elements every component may contain besides widgets, components and bindings */
		const NameSet STRUCTURE = {"if", "else", "repeat"};

		/** Walks the components and collects issues */
		class Checker
		{
		  public:
			/** Check against `reference` */
			explicit Checker(const Reference& reference)
				: m_reference(reference)
			{
				for (const auto& source : reference.sources)
				{
					m_globals.insert(m_globals.end(), source.fields.begin(), source.fields.end());
				}
				for (const auto& presenter : reference.presenters)
				{
					m_presenters.emplace(presenter.name, &presenter);
				}
				for (const auto& widget : reference.widgets)
				{
					m_widgets.emplace(widget.name, &widget);
				}
				for (const auto& action : reference.globalActions)
				{
					m_globalActions.insert(action.name);
				}
				for (const auto& style : reference.styles)
				{
					m_styles.insert(style.name);
				}
				for (const auto& colour : reference.colours)
				{
					m_consts.insert(colour);
				}
				for (const auto& c : reference.consts)
				{
					m_consts.insert(c.name);
				}
				m_fonts.insert(reference.fonts.begin(), reference.fonts.end());
				m_translations.insert(reference.translationKeys.begin(), reference.translationKeys.end());
				m_triggers.insert(reference.triggers.begin(), reference.triggers.end());
			}

			/** Check every component, from the views nothing else uses */
			std::vector<LintIssue> run()
			{
				for (const auto& component : m_reference.components)
				{
					load(component);
				}

				for (const auto& component : m_reference.components)
				{
					auto found = m_components.find(component.name);
					if (found == m_components.end())
					{
						continue;
					}
					const ComponentDef& def = found->second;
					Scope scope;
					if (const Presenter* presenter = presenterNamed(def.presenter))
					{
						scope.fields.push_back(&presenter->fields);
						scope.handlers.push_back(presenter);
					}
					if (component.uses.empty())
					{
						const std::map<std::string, std::string> noProps;
						instance(def, scope, noProps);
					}
					// A placed widget is a view of its own too, given its widget's props and default settings, and
					// with the names of its place (its size) after its presenter's
					for (const auto& use : component.uses)
					{
						if (use.as != Use::As::Widget)
						{
							continue;
						}
						if (const WidgetDef* widget = placeable(use.by))
						{
							Scope placed = scope;
							placed.fields.push_back(&m_reference.placedFields);
							placed.where = "widget " + widget->id;
							for (const auto& settings : settingsToCheck(*widget))
							{
								instance(def, placed, widget->propsFor(settings));
							}
						}
					}
				}
				examples();
				widgetFiles();

				std::sort(m_issues.begin(),
						  m_issues.end(),
						  [](const LintIssue& a, const LintIssue& b)
						  { return std::tie(a.file, a.line, a.message) < std::tie(b.file, b.line, b.message); });
				m_issues.erase(std::unique(m_issues.begin(), m_issues.end()), m_issues.end());
				return std::move(m_issues);
			}

		  private:
			/**
			 * The settings a widget's component is checked with: the defaults, then each option of each choice setting
			 * in turn (the others at their defaults), as a choice can name a font or style (`font_${size}`)
			 */
			static std::vector<nlohmann::json> settingsToCheck(const WidgetDef& widget)
			{
				std::vector<nlohmann::json> all{nlohmann::json::object()};
				for (const auto& setting : widget.settings)
				{
					for (const auto& option : setting.choices())
					{
						if (setting.kind == SettingKind::Choice)
						{
							all.push_back({{setting.name, option.value}});
						}
					}
				}
				return all;
			}

			/** The placeable widget `id`, or nullptr */
			const WidgetDef* placeable(const std::string& id) const
			{
				const auto& widgets = m_reference.placeable;
				const auto it = std::find_if(
					widgets.begin(), widgets.end(), [&id](const WidgetDef& widget) { return widget.id == id; });
				return it == widgets.end() ? nullptr : &*it;
			}

			/**
			 * Check each widget file: its component exists and takes every prop and setting it gives; a row setting's
			 * list is a global list whose rows have its key and label; and every translation key exists
			 */
			void widgetFiles()
			{
				for (const auto& widget : m_reference.placeable)
				{
					const std::string file = widget.source.generic_string();
					const auto report = [&](std::string message)
					{ m_issues.push_back({.file = file, .line = 0, .message = std::move(message)}); };
					const auto translation = [&](const std::string& key)
					{
						if (!key.empty() && !m_translations.contains(key))
						{
							report(fmt::format("no translation key `{:s}`", key));
						}
					};
					translation(widget.titleTag);
					translation(widget.descriptionTag);
					translation(widget.categoryTag);

					auto component = m_components.find(widget.component);
					if (component == m_components.end())
					{
						report(fmt::format("no component `{:s}`", widget.component));
						continue;
					}
					const auto& props = component->second.props;
					for (const auto& [name, value] : widget.props)
					{
						if (!props.contains(name))
						{
							report(fmt::format("`{:s}` takes no prop `{:s}`", widget.component, name));
						}
					}
					for (const auto& setting : widget.settings)
					{
						if (!props.contains(setting.name))
						{
							report(fmt::format(
								"`{:s}` takes no prop `{:s}` for its setting", widget.component, setting.name));
						}
						translation(setting.titleTag);
						for (const auto& option : setting.options)
						{
							translation(option.titleTag);
						}
						if (setting.kind == SettingKind::Row)
						{
							rowSetting(setting, report);
						}
					}
				}
			}

			/**
			 * Check a row setting's list is a global list whose rows have its key and label. For an object model list,
			 * the key must be a field every row has (`index`); the label may also be a field of the item.
			 */
			void rowSetting(const SettingDef& setting, const std::function<void(std::string)>& report) const
			{
				const Context global{.def = nullptr, .props = nullptr, .scope = {}};
				if (const auto omList = omPath(setting.list, global, true))
				{
					Context item = global;
					item.scope.omItems.push_back(omList->index(0));
					if (!isRowField(m_reference.omRowFields, setting.key))
					{
						report(fmt::format("setting `{:s}`: the rows of `{:s}` have no field `{:s}`",
										   setting.name,
										   setting.list,
										   setting.key));
					}
					if (!isRowField(m_reference.omRowFields, setting.label) && !omPath(setting.label, item, false))
					{
						report(fmt::format("setting `{:s}`: the rows of `{:s}` have no field `{:s}`",
										   setting.name,
										   setting.list,
										   setting.label));
					}
					return;
				}
				const auto list = std::find_if(
					m_globals.begin(), m_globals.end(), [&](const Field& field) { return field.name == setting.list; });
				if (list == m_globals.end() || list->rowFields.empty())
				{
					report(fmt::format("setting `{:s}`: no global list `{:s}`", setting.name, setting.list));
					return;
				}
				for (const std::string* field : {&setting.key, &setting.label})
				{
					const bool found = std::any_of(list->rowFields.begin(),
												   list->rowFields.end(),
												   [field](const Field& row) { return row.name == *field; });
					if (!found)
					{
						report(fmt::format("setting `{:s}`: the rows of `{:s}` have no field `{:s}`",
										   setting.name,
										   setting.list,
										   *field));
					}
				}
			}

			/** Whether `fields` has a field (not a list) called `name` */
			static bool isRowField(const std::vector<Field>& fields, const std::string& name)
			{
				return std::any_of(fields.begin(),
								   fields.end(),
								   [&name](const Field& field)
								   { return field.name == name && field.rowFields.empty(); });
			}

			/** Read `component`'s file and what it declares */
			void load(const Component& component)
			{
				std::string error;
				auto root = parseXmlFile(component.source, error);
				if (!root)
				{
					m_issues.push_back({.file = component.source, .line = 0, .message = error});
					return;
				}
				ComponentDef& def = m_components[component.name];
				def = ComponentDef{.name = component.name,
								   .presenter = component.presenter,
								   .file = component.source,
								   .root = std::move(*root)};
				for (const auto& prop : component.props)
				{
					def.props[prop.name] = prop.defaultValue;
				}
				const std::pair<const char*, NameSet*> sections[] = {
					{"styles", &def.styles}, {"subjects", &def.subjects}, {"consts", &def.consts}};
				for (const auto& [section, names] : sections)
				{
					if (const Element* element = def.root.child(section))
					{
						for (const auto& entry : element->children)
						{
							if (const std::string* name = entry.attr("name"))
							{
								names->insert(*name);
							}
						}
					}
				}
			}

			/**
			 * Check each widget's example. An example is a snippet from somewhere in the UI, so its names are looked up
			 * in its sample subjects, every presenter and the globals, and its actions in every presenter.
			 */
			void examples()
			{
				Scope scope;
				for (const auto& presenter : m_reference.presenters)
				{
					scope.fields.push_back(&presenter.fields);
					scope.handlers.push_back(&presenter);
				}
				for (const auto& widget : m_reference.widgets)
				{
					if (widget.example.empty())
					{
						continue;
					}
					// The first line of the example is on the line of the <view> it is put in
					const std::string name = fmt::format("{:s} example", widget.name);
					std::string error;
					auto view = parseXmlText("<view extends=\"lv_obj\">" + widget.example + "</view>", error);
					if (!view)
					{
						m_issues.push_back({.file = name, .line = 0, .message = error});
						continue;
					}
					ComponentDef& def =
						m_examples.emplace_back(ComponentDef{.name = name, .presenter = "", .file = name, .root = {}});
					def.root.tag = "component";
					def.root.children.push_back(std::move(*view));
					for (const auto& subject : widget.exampleSubjects)
					{
						def.subjects.insert(subject.name);
					}
					const std::map<std::string, std::string> noProps;
					instance(def, scope, noProps);
				}
			}

			/** Check one use of `def`, its names looked up in `scope`, with `props` given */
			void instance(const ComponentDef& def, const Scope& scope, const std::map<std::string, std::string>& props)
			{
				// A component that contains itself would recurse forever; its first use is checked
				if (std::find(m_stack.begin(), m_stack.end(), def.name) != m_stack.end())
				{
					return;
				}
				m_stack.push_back(def.name);

				Context context{.def = &def, .props = &props, .scope = scope};
				context.scope.where = scope.where.empty() ? def.name : scope.where + " › " + def.name;

				if (const Element* styles = def.root.child("styles"))
				{
					for (const auto& style : styles->children)
					{
						checkConsts(style, context);
					}
				}

				if (const Element* view = def.root.child("view"))
				{
					checkView(*view, context);
				}
				else
				{
					issue(context, def.root, "has no <view>");
				}
				m_stack.pop_back();
			}

			/** Check a component's `<view>`: its base (a widget, or a component given the view's attributes as props)
			 */
			void checkView(const Element& view, const Context& context)
			{
				const std::string* extends = view.attr("extends");
				const std::string base = extends ? *extends : "lv_obj";
				auto component = m_components.find(base);
				if (component != m_components.end())
				{
					instance(component->second, context.scope, propsOf(view, component->second, context));
				}
				else if (isWidget(base))
				{
					checkAttributes(view, base, context);
				}
				else
				{
					issue(context, view, fmt::format("extends `{:s}`, which is no widget or component", base));
				}
				for (const auto& child : view.children)
				{
					checkElement(child, context);
				}
			}

			/** Check an element inside a view, and what it contains */
			void checkElement(const Element& element, const Context& context)
			{
				const std::string& tag = element.tag;
				if (tag == "action")
				{
					checkAttributes(element, "lv_obj-action", context);
					return;
				}
				if (tag == "style")
				{
					if (auto name = valueOf(element, "name", context))
					{
						checkStyle(*name, element, context);
					}
					return;
				}
				if (tag.starts_with("bind_"))
				{
					checkBinding(element, context);
					return;
				}
				if (STRUCTURE.contains(tag))
				{
					checkStructure(element, context);
					return;
				}

				auto component = m_components.find(tag);
				if (component != m_components.end())
				{
					instance(component->second, context.scope, propsOf(element, component->second, context));
				}
				else if (isWidget(tag))
				{
					checkAttributes(element, tag, context);
				}
				else
				{
					issue(context, element, fmt::format("<{:s}> is no widget, component or binding", tag));
				}
				for (const auto& child : element.children)
				{
					checkElement(child, context);
				}
			}

			/** Check a `<bind_*>` element: its subject, style and condition */
			void checkBinding(const Element& element, const Context& context)
			{
				if (!isWidget("lv_obj-" + element.tag))
				{
					issue(context, element, fmt::format("<{:s}> is no binding", element.tag));
					return;
				}
				if (auto subject = valueOf(element, "subject", context))
				{
					checkSubject(*subject, element, context);
				}
				if (auto cond = valueOf(element, "cond", context))
				{
					checkExpression(*cond, element, context);
				}
				if (element.tag.starts_with("bind_style"))
				{
					if (auto name = valueOf(element, "name", context))
					{
						checkStyle(*name, element, context);
					}
				}
				checkConsts(element, context);
			}

			/** Check `<if>`, `<else>` and `<repeat>`, and what they contain */
			void checkStructure(const Element& element, const Context& context)
			{
				if (auto cond = valueOf(element, "cond", context))
				{
					checkExpression(*cond, element, context);
				}
				if (auto count = valueOf(element, "count", context))
				{
					if (!std::all_of(count->begin(), count->end(), [](char c) { return std::isdigit(c); }))
					{
						checkSubject(*count, element, context);
					}
				}
				for (const auto& child : element.children)
				{
					checkElement(child, context);
				}
			}

			/** Check the attributes of widget `widget` written as `element`, then follow the lists and views it shows
			 */
			void checkAttributes(const Element& element, const std::string& widget, const Context& context)
			{
				checkConsts(element, context);
				for (const auto& [name, raw] : element.attrs)
				{
					if (auto value = resolve(raw, element, context))
					{
						check(kindOf(widget, name), *value, element, context);
					}
				}

				if (widget == "ds_list")
				{
					auto list = valueOf(element, "list", context);
					auto item = valueOf(element, "item", context);
					const Field* field = list ? findField(*list, context) : nullptr;
					auto component = item ? m_components.find(*item) : m_components.end();
					const auto omList = field || !list ? std::nullopt : omPath(*list, context, true);
					if (auto key = valueOf(element, "key", context); key && (field || omList))
					{
						// The rows of an object model list are keyed by a field every row has, e.g. `index`
						const Field omRows{
							.name = *list, .type = "list", .description = "", .rowFields = m_reference.omRowFields};
						checkKey(*key, field ? *field : omRows, element, context);
					}
					if (component != m_components.end() && (field || omList))
					{
						// An object model list's rows have names relative to their item besides their fields
						Scope scope = context.scope;
						scope.fields.insert(scope.fields.begin(), field ? &field->rowFields : &m_reference.omRowFields);
						if (omList)
						{
							scope.omItems.insert(scope.omItems.begin(), omList->index(0));
						}
						const std::map<std::string, std::string> noProps;
						instance(component->second, scope, noProps);
					}
				}
				else if (widget == "ds_scope")
				{
					auto item = valueOf(element, "item", context);
					auto path = valueOf(element, "path", context);
					auto element_ = path ? omPath(*path, context, false) : std::nullopt;
					auto component = item ? m_components.find(*item) : m_components.end();
					if (component != m_components.end() && element_)
					{
						Scope scope = context.scope;
						scope.omItems.insert(scope.omItems.begin(), *element_);
						const std::map<std::string, std::string> noProps;
						instance(component->second, scope, noProps);
					}
				}
				else if (widget == "ds_view")
				{
					auto view = valueOf(element, "view", context);
					auto component = view ? m_components.find(*view) : m_components.end();
					if (component != m_components.end())
					{
						auto presenterName = valueOf(element, "presenter", context);
						const Presenter* presenter =
							presenterNamed(presenterName.value_or(component->second.presenter));
						// A view's names are its presenter's and the globals; its actions also go to outer views
						Scope scope{.fields = {}, .handlers = context.scope.handlers, .where = context.scope.where};
						if (presenter)
						{
							scope.fields.push_back(&presenter->fields);
							scope.handlers.insert(scope.handlers.begin(), presenter);
						}
						const std::map<std::string, std::string> noProps;
						instance(component->second, scope, noProps);
					}
				}
			}

			/** Check `value`, the value of an attribute of kind `kind` */
			void check(AttributeKind kind, const std::string& value, const Element& element, const Context& context)
			{
				switch (kind)
				{
				case AttributeKind::Text:
					return;
				case AttributeKind::Subject:
					checkSubject(value, element, context);
					return;
				case AttributeKind::List:
					if (const Field* field = findField(value, context); !field || field->rowFields.empty())
					{
						if (!isLocalSubject(value, context) && !omPath(value, context, true))
						{
							issue(context, element, fmt::format("no list `{:s}`", value));
						}
					}
					return;
				case AttributeKind::Component:
				case AttributeKind::View:
					if (!m_components.contains(value))
					{
						issue(context, element, fmt::format("no component `{:s}`", value));
					}
					return;
				case AttributeKind::Presenter:
					if (!presenterNamed(value))
					{
						issue(context, element, fmt::format("no presenter `{:s}`", value));
					}
					return;
				case AttributeKind::Style:
					checkStyle(value, element, context);
					return;
				case AttributeKind::Translation:
					checkTranslation(value, element, context);
					return;
				case AttributeKind::Action:
					checkAction(value, element, context);
					return;
				case AttributeKind::ActionArg:
					checkPlaceholders(value, element, context);
					return;
				case AttributeKind::Trigger:
					if (!m_triggers.contains(value))
					{
						issue(context, element, fmt::format("no trigger `{:s}`", value));
					}
					return;
				}
			}

			/** What the value of attribute `attr` of widget `widget` names */
			AttributeKind kindOf(const std::string& widget, const std::string& attr) const
			{
				// The widget's spec, and the specs of the DuetScreen widgets it extends
				for (auto it = m_widgets.find(widget); it != m_widgets.end(); it = m_widgets.find(it->second->extends))
				{
					for (const auto& attribute : it->second->attributes)
					{
						if (attribute.name == attr)
						{
							return attribute.kind;
						}
					}
				}
				// The XML engine's conventions for its own widgets
				if (attr.starts_with("bind_") && attr.find('-') == std::string::npos)
				{
					return AttributeKind::Subject;
				}
				if (attr == "translation_tag")
				{
					return AttributeKind::Translation;
				}
				if (widget == "lv_spangroup-span" && attr == "style")
				{
					return AttributeKind::Style;
				}
				return AttributeKind::Text;
			}

			/** Report a subject `name` that can't be found */
			void checkSubject(const std::string& name, const Element& element, const Context& context)
			{
				if (findField(name, context) || isLocalSubject(name, context))
				{
					return;
				}
				if (!omPath(name, context, false))
				{
					issue(context, element, fmt::format("no subject `{:s}`", name));
					return;
				}
				// An object model name: its converter must exist, and the names of its `{…}` indices too
				const auto parsed = OmName::parse(name);
				const Converter* converter = findConverter(parsed->converter);
				if (!converter)
				{
					issue(context, element, fmt::format("`{:s}`: no converter `{:s}`", name, parsed->converter));
				}
				else if (converter->argumentKind == Converter::Argument::translationKey)
				{
					checkTranslation(parsed->argument, element, context);
				}
				else if (converter->argumentKind == Converter::Argument::translationGroup)
				{
					checkTranslationGroup(parsed->argument, element, context);
				}
				for (const auto& segment : OmTemplate::parse(parsed->path)->segments())
				{
					if (const auto* ref = std::get_if<OmTemplate::Ref>(&segment))
					{
						checkSubject(ref->name, element, context);
					}
				}
			}

			/** `path` with every `{…}` index as 0: any item of an array has the same fields */
			static OM::Path anyItem(const OmTemplate& path)
			{
				OM::Path result;
				for (const auto& segment : path.segments())
				{
					if (const auto* name = std::get_if<std::string>(&segment))
					{
						result = result.field(*name);
					}
					else if (const auto* index = std::get_if<size_t>(&segment))
					{
						result = result.index(*index);
					}
					else
					{
						result = result.index(0);
					}
				}
				return result;
			}

			/**
			 * The object model path `name` stands for (see Om/OmName.h): an `om.` path, or inside object model items, a
			 * path relative to the innermost that has it (`value|…` is the innermost item itself). With a schema, only
			 * a path it lists, and for a `list`, only an array.
			 * @return the path, any `{…}` index as 0, or nothing if `name` isn't an object model name here
			 */
			static std::optional<OM::Path> omPath(const std::string& name, const Context& context, bool list)
			{
				const auto parsed = OmName::parse(name);
				if (!parsed || (list && !parsed->converter.empty()))
				{
					return std::nullopt;
				}
				const auto& schema = OM::Schema::get();
				/* With a schema, only a path it lists, and for a list, only an array */
				const auto fits = [&](const OM::Path& path)
				{
					if (schema.empty())
					{
						return true;
					}
					const auto type = schema.typeAt(path);
					return type && (!list || *type == "any" || type->ends_with("[]"));
				};
				if (parsed->absolute)
				{
					const auto parsedPath = OmTemplate::parse(parsed->path);
					if (parsedPath && fits(anyItem(*parsedPath)))
					{
						return anyItem(*parsedPath);
					}
					return std::nullopt;
				}
				if (parsed->path == "value" && !parsed->converter.empty() && !list && !context.scope.omItems.empty())
				{
					return context.scope.omItems.front();
				}
				// Relative: the innermost item that has the field, as the UI looks it up
				const auto parsedPath = OmTemplate::parse(parsed->path);
				for (const auto& item : context.scope.omItems)
				{
					if (parsedPath && fits(item / anyItem(*parsedPath)))
					{
						return item / anyItem(*parsedPath);
					}
				}
				return std::nullopt;
			}

			/**
			 * The end of the expression name starting at `start`, as the engine reads one (helix-xml's lv_xml_expr.c):
			 * letters, digits, `_` and `.`, brackets and braces with what is inside them, `|` before a word and `:`
			 * before an argument
			 */
			static size_t nameEnd(const std::string& expression, size_t start)
			{
				size_t i = start;
				const auto isNameChar = [](char c)
				{ return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.'; };
				while (i < expression.size())
				{
					const char c = expression[i];
					if (isNameChar(c))
					{
						i++;
					}
					else if (c == '[' || c == '{')
					{
						int depth = 0;
						size_t j = i;
						for (; j < expression.size(); j++)
						{
							if (expression[j] == '[' || expression[j] == '{')
							{
								depth++;
							}
							else if ((expression[j] == ']' || expression[j] == '}') && --depth == 0)
							{
								break;
							}
						}
						if (j == expression.size())
						{
							break;
						}
						i = j + 1;
					}
					else if (c == '|' && i + 1 < expression.size() &&
							 (std::isalpha(static_cast<unsigned char>(expression[i + 1])) || expression[i + 1] == '_'))
					{
						i++;
					}
					else if (c == ':' && i + 1 < expression.size() &&
							 (isNameChar(expression[i + 1]) ||
							  (expression[i + 1] == '-' && i + 2 < expression.size() &&
							   std::isdigit(static_cast<unsigned char>(expression[i + 2])))))
					{
						i += expression[i + 1] == '-' ? 2 : 1;
					}
					else
					{
						break;
					}
				}
				return i;
			}

			/** Report a `<ds_list key="…">` that isn't a field of the rows of `list`, or is one of their lists */
			void checkKey(const std::string& key, const Field& list, const Element& element, const Context& context)
			{
				const auto row = std::find_if(list.rowFields.begin(),
											  list.rowFields.end(),
											  [&key](const Field& field) { return field.name == key; });
				if (row == list.rowFields.end())
				{
					issue(context, element, fmt::format("rows of `{:s}` have no field `{:s}`", list.name, key));
				}
				else if (!row->rowFields.empty())
				{
					issue(context, element, fmt::format("`{:s}` is a list, so it can't be a key", key));
				}
			}

			/** Report the operands of expression `expression` that aren't subjects */
			void checkExpression(const std::string& expression, const Element& element, const Context& context)
			{
				for (size_t i = 0; i < expression.size();)
				{
					const char c = expression[i];
					if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
					{
						const size_t end = nameEnd(expression, i);
						const std::string operand = expression.substr(i, end - i);
						if (!EXPRESSION_WORDS.contains(operand))
						{
							checkSubject(operand, element, context);
						}
						i = end;
					}
					else if (std::isdigit(static_cast<unsigned char>(c)))
					{
						while (i < expression.size() && std::isalnum(static_cast<unsigned char>(expression[i])))
						{
							i++;
						}
					}
					else
					{
						i++;
					}
				}
			}

			/** Report the `{name}` placeholders of an action's `arg` that aren't subjects (`{event.value}` is the edit)
			 */
			void checkPlaceholders(const std::string& arg, const Element& element, const Context& context)
			{
				for (size_t open = arg.find('{'); open != std::string::npos; open = arg.find('{', open + 1))
				{
					const size_t close = arg.find('}', open);
					if (close == std::string::npos)
					{
						return;
					}
					const std::string name = arg.substr(open + 1, close - open - 1);
					if (name != "event.value")
					{
						checkSubject(name, element, context);
					}
				}
			}

			/** Report a translation key, written `tr.<key>`, that isn't written so or isn't in the translations */
			void checkTranslation(const std::string& tag, const Element& element, const Context& context)
			{
				const auto key = Translations::key(tag);
				if (!key)
				{
					issue(context,
						  element,
						  fmt::format("`{:s}`: translation keys are written `{:s}<key>`", tag, Translations::PREFIX));
				}
				else if (!m_translations.contains(*key))
				{
					issue(context, element, fmt::format("no translation `{:s}`", tag));
				}
			}

			/** Report the start of translation keys, written `tr.<group>`, that isn't written so or starts none */
			void checkTranslationGroup(const std::string& tag, const Element& element, const Context& context)
			{
				const auto key = Translations::key(tag);
				if (!key)
				{
					issue(context,
						  element,
						  fmt::format("`{:s}`: translation keys are written `{:s}<key>`", tag, Translations::PREFIX));
					return;
				}
				const std::string group = std::string(*key) + ".";
				const auto it = m_translations.lower_bound(group);
				if (it == m_translations.end() || !it->starts_with(group))
				{
					issue(context, element, fmt::format("no translations `{:s}.…`", tag));
				}
			}

			/** Report an action that no enclosing presenter, nor the globals, handle */
			void checkAction(const std::string& name, const Element& element, const Context& context)
			{
				if (!name.starts_with(Actions::PRESENTER_PREFIX) && !name.starts_with(Actions::GLOBAL_PREFIX))
				{
					issue(context,
						  element,
						  fmt::format("`{:s}`: actions are written `{:s}<action>` or `{:s}<group>.<action>`",
									  name,
									  Actions::PRESENTER_PREFIX,
									  Actions::GLOBAL_PREFIX));
					return;
				}
				if (m_globalActions.contains(name))
				{
					return;
				}
				for (const Presenter* presenter : context.scope.handlers)
				{
					for (const auto& action : presenter->actions)
					{
						if (action.name == name)
						{
							return;
						}
					}
				}
				issue(context, element, fmt::format("no action `{:s}`", name));
			}

			/** Report a style that is neither the component's nor the theme's */
			void checkStyle(const std::string& name, const Element& element, const Context& context)
			{
				if (!context.def->styles.contains(name) && !m_styles.contains(name))
				{
					issue(context, element, fmt::format("no style `{:s}`", name));
				}
			}

			/** Report `#name` values in `element`'s attributes that are no const or colour, and unknown fonts */
			void checkConsts(const Element& element, const Context& context)
			{
				for (const auto& [name, raw] : element.attrs)
				{
					auto value = resolve(raw, element, context);
					if (!value)
					{
						continue;
					}
					if (value->starts_with('#') && !isHexColour(*value))
					{
						const std::string constName = value->substr(1);
						if (!m_consts.contains(constName) && !context.def->consts.contains(constName))
						{
							issue(context, element, fmt::format("no const or colour `{:s}`", *value));
						}
					}
					if (name.ends_with("text_font") && !m_fonts.contains(*value))
					{
						issue(context, element, fmt::format("no font `{:s}`", *value));
					}
				}
			}

			/** The value of `element`'s attribute `name`, with its prop resolved; nullopt if unset or empty */
			std::optional<std::string> valueOf(const Element& element, std::string_view name, const Context& context)
			{
				const std::string* raw = element.attr(name);
				return raw ? resolve(*raw, element, context) : std::nullopt;
			}

			/**
			 * `raw` with its props replaced by the values given: a whole `$prop`, or each `${prop}` inside a longer
			 * value; nullopt if that leaves nothing to check
			 */
			std::optional<std::string> resolve(const std::string& raw, const Element& element, const Context& context)
			{
				if (raw.starts_with('$') && !raw.starts_with("${"))
				{
					return propValue(raw.substr(1), element, context);
				}
				std::string result;
				size_t from = 0;
				for (size_t start = raw.find("${"); start != std::string::npos; start = raw.find("${", from))
				{
					const size_t end = raw.find('}', start);
					if (end == std::string::npos)
					{
						break;
					}
					const auto value = propValue(raw.substr(start + 2, end - start - 2), element, context);
					if (!value)
					{
						return std::nullopt;
					}
					result += raw.substr(from, start - from) + *value;
					from = end + 1;
				}
				result += raw.substr(from);
				return result.empty() ? std::nullopt : std::optional<std::string>(result);
			}

			/** The value of prop `prop` given where `context` is used, else its default; nullopt if none or empty */
			std::optional<std::string> propValue(const std::string& prop,
												 const Element& element,
												 const Context& context)
			{
				auto declared = context.def->props.find(prop);
				if (declared == context.def->props.end())
				{
					issue(context, element, fmt::format("no prop `{:s}`", prop));
					return std::nullopt;
				}
				auto given = context.props->find(prop);
				const std::string& value = given != context.props->end() ? given->second : declared->second;
				return value.empty() ? std::nullopt : std::optional<std::string>(value);
			}

			/** The props `element` gives `component`, resolved where `element` is written */
			std::map<std::string, std::string> propsOf(const Element& element,
													   const ComponentDef& component,
													   const Context& context)
			{
				std::map<std::string, std::string> props;
				for (const auto& [name, raw] : element.attrs)
				{
					if (component.props.contains(name))
					{
						if (auto value = resolve(raw, element, context))
						{
							props[name] = *value;
						}
					}
				}
				return props;
			}

			/** The field called `name` in the scope, the innermost first, then the globals; nullptr if none */
			const Field* findField(const std::string& name, const Context& context) const
			{
				for (const auto* fields : context.scope.fields)
				{
					for (const auto& field : *fields)
					{
						if (field.name == name)
						{
							return &field;
						}
					}
				}
				for (const auto& field : m_globals)
				{
					if (field.name == name)
					{
						return &field;
					}
				}
				return nullptr;
			}

			/** Whether `value` is a colour written in hex, e.g. "#ff0000", rather than a `#const` */
			static bool isHexColour(const std::string& value)
			{
				const size_t digits = value.size() - 1;
				return (digits == 3 || digits == 6 || digits == 8) &&
					   std::all_of(value.begin() + 1, value.end(), [](char c) { return std::isxdigit(c); });
			}

			/** Whether `name` is a subject the component declares itself */
			static bool isLocalSubject(const std::string& name, const Context& context)
			{
				return context.def->subjects.contains(name);
			}

			/** The presenter called `name`, or nullptr */
			const Presenter* presenterNamed(const std::string& name) const
			{
				auto it = m_presenters.find(name);
				return it == m_presenters.end() ? nullptr : it->second;
			}

			/** Whether the XML engine has an element called `name` */
			static bool isWidget(const std::string& name)
			{
				return lv_xml_widget_get_processor(name.c_str()) != nullptr;
			}

			/** Record a problem with `element` of the component in `context` */
			void issue(const Context& context, const Element& element, std::string message)
			{
				m_issues.push_back({.file = context.def->file,
									.line = element.line,
									.message = fmt::format("{:s} (in {:s})", message, context.scope.where)});
			}

			const Reference& m_reference;									 // What the UI provides
			std::vector<Field> m_globals;									 // Every data source's fields
			std::map<std::string, const Presenter*> m_presenters;			 // Presenters by name
			std::map<std::string, const WidgetSpec*, std::less<>> m_widgets; // Widget specs by name
			NameSet m_globalActions;										 // Global action names
			NameSet m_styles;												 // Theme style names
			NameSet m_consts;												 // Theme const and colour names
			NameSet m_fonts;												 // Font names
			NameSet m_translations;											 // Translation keys
			NameSet m_triggers;												 // Action trigger names
			std::map<std::string, ComponentDef, std::less<>> m_components;	 // Components by name
			std::deque<ComponentDef> m_examples; // The widgets' examples, as components; a deque keeps them in place
			std::vector<std::string> m_stack;	 // Components being checked, outermost first
			std::vector<LintIssue> m_issues;	 // Problems found
		};
	} // namespace

	std::vector<LintIssue> lint(const Reference& reference)
	{
		return Checker(reference).run();
	}

	std::string toString(const LintIssue& issue)
	{
		return fmt::format("{:s}:{:d}: {:s}", issue.file, issue.line, issue.message);
	}
} // namespace UI::Xml::Api

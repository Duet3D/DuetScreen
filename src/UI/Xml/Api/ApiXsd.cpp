/*
 * ApiXsd.cpp
 */

#include "ApiXsd.h"
#include "ApiLvgl.h"
#include <algorithm>
#include <fmt/format.h>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace UI::Xml::Api
{
	namespace
	{
		/** `text` escaped for an XML attribute or text */
		std::string escape(std::string_view text)
		{
			std::string result;
			for (const char c : text)
			{
				switch (c)
				{
				case '&':
					result += "&amp;";
					break;
				case '<':
					result += "&lt;";
					break;
				case '>':
					result += "&gt;";
					break;
				case '"':
					result += "&quot;";
					break;
				default:
					result += c;
				}
			}
			return result;
		}

		/** The schema type for an attribute of kind `kind`; names offer every name of that kind */
		std::string typeOf(AttributeKind kind)
		{
			switch (kind)
			{
			case AttributeKind::Subject:
				return "subject";
			case AttributeKind::List:
				return "list";
			case AttributeKind::Component:
				return "component";
			case AttributeKind::View:
				return "view";
			case AttributeKind::Presenter:
				return "presenter";
			case AttributeKind::Style:
				return "style";
			case AttributeKind::Translation:
				return "translation";
			case AttributeKind::Action:
				return "action";
			case AttributeKind::Trigger:
				return "trigger";
			case AttributeKind::Text:
			case AttributeKind::ActionArg:
				break;
			}
			return "xs:string";
		}

		/** Add every field in `fields`, and in their rows, to `subjects`, and the lists among them to `lists` */
		void collectFields(const std::vector<Field>& fields,
						   std::set<std::string>& subjects,
						   std::set<std::string>& lists)
		{
			for (const auto& field : fields)
			{
				subjects.insert(field.name);
				if (!field.rowFields.empty())
				{
					lists.insert(field.name);
					collectFields(field.rowFields, subjects, lists);
				}
			}
		}

		/** The element `name`, a registered widget or element, is written as: `lv_obj-x`, a child of any widget, as `x`
		 */
		std::string elementName(std::string_view name)
		{
			constexpr std::string_view anyWidgetChild = "lv_obj-";
			if (name.starts_with(anyWidgetChild))
			{
				name.remove_prefix(anyWidgetChild.size());
			}
			return std::string(name);
		}

		/** Writes the schema */
		class Writer
		{
		  public:
			/** Write the schema for `reference`, with what `lvgl` describes of the engine's own elements */
			Writer(const Reference& reference, const LvglApi& lvgl)
				: m_reference(reference)
				, m_lvgl(lvgl)
			{
				for (const auto& widget : m_reference.widgets)
				{
					m_widgets.emplace(widget.name, &widget);
				}
				for (const auto& element : m_lvgl.elements)
				{
					m_lvglElements.emplace(element.name, &element);
				}
				for (const auto& component : m_reference.components)
				{
					m_components.emplace(component.name, &component);
				}
				for (const auto& type : m_lvgl.enums)
				{
					m_enums.insert(type.name);
				}
			}

			/** The schema */
			std::string write()
			{
				m_out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
						 "<!-- Generated from the code by tests/src/test_cases/UI/test_xml_ui_api.cpp, like "
						 "XML_UI_API.md. Don't edit it. -->\n"
						 "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n";
				writeNameTypes();
				writeLvglTypes();
				writeOpenContent();
				collectAttributeGroups();
				writeAttributeGroups();
				writeStructure();
				writeWidgets();
				writeLvglElements();
				writeComponents();
				m_out += "</xs:schema>\n";
				return std::move(m_out);
			}

		  private:
			/** For each kind of name, a type that offers every name of the kind and accepts any value */
			void writeNameTypes()
			{
				std::set<std::string> subjects;
				std::set<std::string> lists;
				for (const auto& source : m_reference.sources)
				{
					collectFields(source.fields, subjects, lists);
				}
				std::set<std::string> actions;
				for (const auto& action : m_reference.globalActions)
				{
					actions.insert(action.name);
				}
				std::set<std::string> presenters;
				for (const auto& presenter : m_reference.presenters)
				{
					presenters.insert(presenter.name);
					collectFields(presenter.fields, subjects, lists);
					for (const auto& action : presenter.actions)
					{
						actions.insert(action.name);
					}
				}
				std::set<std::string> components;
				for (const auto& component : m_reference.components)
				{
					components.insert(component.name);
				}
				const std::set<std::string>& views = components;
				std::set<std::string> widgets = components;
				for (const auto& widget : m_reference.widgets)
				{
					widgets.insert(widget.name);
				}
				for (const auto& element : m_reference.lvglElements)
				{
					if (element.find('-') == std::string::npos) // A widget, not a child element
					{
						widgets.insert(element);
					}
				}
				std::set<std::string> styles;
				for (const auto& style : m_reference.styles)
				{
					styles.insert(style.name);
				}

				writeNameType("subject", subjects);
				writeNameType("list", lists);
				writeNameType("action", actions);
				writeNameType("trigger", {m_reference.triggers.begin(), m_reference.triggers.end()});
				writeNameType("presenter", presenters);
				writeNameType("component", components);
				writeNameType("view", views);
				writeNameType("widget", widgets);
				writeNameType("style", styles);
				writeNameType("translation", {m_reference.translationKeys.begin(), m_reference.translationKeys.end()});
				writeNameType("font", {m_reference.fonts.begin(), m_reference.fonts.end()});
				writeNameType("image", {m_reference.images.begin(), m_reference.images.end()});
			}

			/** Type `name`: any string, offering `names` */
			void writeNameType(std::string_view name, const std::set<std::string>& names)
			{
				std::vector<std::pair<std::string, std::string>> values;
				for (const auto& value : names)
				{
					values.emplace_back(value, "");
				}
				writeDocumentedNameType(name, values);
			}

			/** Type `name`: any string, offering each of `values`, documented with its help where it has one */
			void writeDocumentedNameType(std::string_view name,
										 const std::vector<std::pair<std::string, std::string>>& values)
			{
				fmt::format_to(std::back_inserter(m_out),
							   "\t<xs:simpleType name=\"{0:s}.names\">\n\t\t<xs:restriction base=\"xs:string\">\n",
							   name);
				for (const auto& [value, help] : values)
				{
					if (help.empty())
					{
						fmt::format_to(
							std::back_inserter(m_out), "\t\t\t<xs:enumeration value=\"{:s}\"/>\n", escape(value));
						continue;
					}
					fmt::format_to(std::back_inserter(m_out),
								   "\t\t\t<xs:enumeration value=\"{:s}\">"
								   "<xs:annotation><xs:documentation>{:s}</xs:documentation></xs:annotation>"
								   "</xs:enumeration>\n",
								   escape(value),
								   escape(help));
				}
				fmt::format_to(
					std::back_inserter(m_out),
					"\t\t</xs:restriction>\n\t</xs:simpleType>\n"
					"\t<xs:simpleType name=\"{0:s}\">\n\t\t<xs:union memberTypes=\"{0:s}.names xs:string\"/>\n"
					"\t</xs:simpleType>\n",
					name);
			}

			/** A type for each enum the engine's elements take, and for its booleans and selectors */
			void writeLvglTypes()
			{
				std::vector<std::pair<std::string, std::string>> selectors;
				for (const auto& type : m_lvgl.enums)
				{
					writeDocumentedNameType(type.name, type.values);
					if (type.name == "lv_part" || type.name == "lv_state")
					{
						selectors.insert(selectors.end(), type.values.begin(), type.values.end());
					}
				}
				writeNameType("bool", std::set<std::string>{"false", "true"});
				// Besides a number of px, or a percentage
				writeDocumentedNameType("size", {{"content", "As big as its content"}});
				writeDocumentedNameType("radius", {{"circle", "Fully round"}});
				writeDocumentedNameType("selector", selectors);
			}

			/** The content every widget may have: any elements, any attributes (its base widget's) */
			void writeOpenContent()
			{
				m_out += "\t<xs:group name=\"anyContent\">\n\t\t<xs:sequence>\n"
						 "\t\t\t<xs:any minOccurs=\"0\" maxOccurs=\"unbounded\" processContents=\"lax\"/>\n"
						 "\t\t</xs:sequence>\n\t</xs:group>\n";
			}

			/** The schema type for an attribute of the engine's: a name type where it names something */
			std::string lvglTypeOf(const LvglAttribute& attribute) const
			{
				if (attribute.name == "translation_tag" || attribute.name == "placeholder_tag")
				{
					return "translation";
				}
				const std::string& type = attribute.type;
				if (type == "subject" || type == "style" || type == "font" || type == "image" || type == "radius")
				{
					return type;
				}
				if (type == "coords")
				{
					return "size";
				}
				if (type == "bool" || type.starts_with("flag:"))
				{
					return "bool";
				}
				if (type == "selector" || type == "selector+")
				{
					return "selector";
				}
				// `enum:lv_align`, or `enum:lv_chart_axis+` for several ORed
				constexpr std::string_view enumPrefix = "enum:";
				if (type.starts_with(enumPrefix))
				{
					const std::string name =
						type.substr(enumPrefix.size(), type.find_first_of("+[") - enumPrefix.size());
					if (m_enums.contains(name))
					{
						return name;
					}
				}
				return "xs:string";
			}

			/**
			 * The attributes each widget, element and component takes of its own, by name: what an element of that
			 * name takes, before what it takes from what it extends. `styledef` is what a `<style>` takes.
			 */
			void collectAttributeGroups()
			{
				for (const auto& element : m_lvgl.elements)
				{
					std::vector<Attribute>& group = m_groups[element.name];
					for (const auto& attribute : element.attributes)
					{
						group.push_back({attribute.name, attribute.help, lvglTypeOf(attribute)});
					}
				}
				std::vector<Attribute>& styleProperties = m_groups["styledef"];
				for (const auto& property : m_lvgl.styleProperties)
				{
					styleProperties.push_back({property.name, property.help, lvglTypeOf(property)});
				}
				for (const auto& widget : m_reference.widgets)
				{
					std::vector<Attribute>& group = m_groups[widget.name];
					for (const auto& attribute : widget.attributes)
					{
						group.push_back({attribute.name, attribute.description, typeOf(attribute.kind)});
					}
				}
				for (const auto& component : m_reference.components)
				{
					std::vector<Attribute>& group = m_groups[component.name];
					for (const auto& prop : component.props)
					{
						group.push_back(
							{prop.name, fmt::format("Prop `{:s}` ({:s})", prop.name, prop.type), "xs:string"});
					}
				}
			}

			/** Each group of attributes, to be referred to by every element that takes it */
			void writeAttributeGroups()
			{
				for (const auto& [name, attributes] : m_groups)
				{
					if (attributes.empty())
					{
						continue;
					}
					fmt::format_to(std::back_inserter(m_out), "\t<xs:attributeGroup name=\"{:s}\">\n", groupName(name));
					std::set<std::string> seen;
					for (const auto& attribute : attributes)
					{
						if (seen.insert(attribute.name).second)
						{
							writeAttribute(attribute, "\t\t");
						}
					}
					m_out += "\t</xs:attributeGroup>\n";
				}
			}

			/** The schema name of the attribute group of `name` */
			static std::string groupName(std::string_view name) { return fmt::format("{:s}.attributes", name); }

			/**
			 * The groups of attributes an element called `name` takes: its own, then those of what it extends, in
			 * turn. A DuetScreen widget extends its spec's base, a component what its view extends, and every widget
			 * of the engine's extends lv_obj.
			 */
			std::vector<std::string> chainOf(const std::string& name) const
			{
				std::vector<std::string> chain;
				std::set<std::string> visited;
				for (std::string current = name; !current.empty() && visited.insert(current).second;)
				{
					chain.push_back(current);
					if (const auto widget = m_widgets.find(current); widget != m_widgets.end())
					{
						current = widget->second->extends;
					}
					else if (const auto element = m_lvglElements.find(current); element != m_lvglElements.end())
					{
						current = element->second->isObject && current != "lv_obj" ? "lv_obj" : "";
					}
					else if (const auto component = m_components.find(current); component != m_components.end())
					{
						current = component->second->extends;
					}
					else
					{
						current.clear();
					}
				}
				return chain;
			}

			/** `<component>` and its sections, `<prop>`, `<view>` and `<style>` */
			void writeStructure()
			{
				writeElement(
					"component",
					"A component: an `<api>` of props, `<consts>`, `<styles>`, `<subjects>`, and its `<view>`.",
					{{"presenter", "The presenter that gives its view names and handles its actions", "presenter"}});
				writeElement("api", "The props the component takes", {});
				writeElement("consts", "The component's consts, written `#name` in its attributes", {});
				writeElement("styles", "The component's styles, each a `<style name=\"…\" …/>`", {});
				writeElement("subjects", "The component's own subjects", {});
				writeElement("prop",
							 "A prop: `<my_component title=\"…\"/>` sets `$title` inside the component",
							 {{"name", "Its name", "xs:string"},
							  {"type", "Its type, e.g. string", "xs:string"},
							  {"default", "Its value when not given", "xs:string"}});
				// The widget it extends isn't known here, so it is offered lv_obj's attributes
				writeElement("view",
							 "The component's root widget. Takes every attribute of the widget it extends.",
							 {{"extends", "The widget, or component, it is (default lv_obj)", "widget"}},
							 {"lv_obj"});
				// One element in two places: added to a widget, or defined in <styles> with style properties
				writeElement(
					"style",
					"Inside a widget: add the style called `name`. Inside `<styles>`: define a style.",
					{{"name", "The style", "style"}, {"selector", "The part and state it applies to", "selector"}},
					{"lv_obj-style", "styledef"});
			}

			/** The DuetScreen widgets, `<action>` and the other elements with specs */
			void writeWidgets()
			{
				for (const auto& widget : m_reference.widgets)
				{
					std::string doc = widget.description;
					if (!widget.extends.empty())
					{
						doc += fmt::format(" Also takes every {:s} attribute.", widget.extends);
					}
					writeElement(elementName(widget.name), doc, {}, chainOf(widget.name));
				}
			}

			/**
			 * The XML engine's own widgets and elements, as it registers them, with what its descriptions say of
			 * them. Declaring them lets an editor complete what is written inside them, too.
			 */
			void writeLvglElements()
			{
				for (const auto& element : m_reference.lvglElements)
				{
					const std::string name = elementName(element);
					const auto described = m_lvglElements.find(element);
					std::string doc;
					if (described != m_lvglElements.end() && !described->second->help.empty())
					{
						doc = described->second->help + ".";
					}
					else if (name != element)
					{
						doc = fmt::format("LVGL's `<{:s}>`, inside any widget.", name);
					}
					else if (const auto dash = element.find('-'); dash != std::string::npos)
					{
						doc = fmt::format("LVGL's `<{:s}>`, inside `<{:s}>`.", element, element.substr(0, dash));
					}
					else
					{
						doc = fmt::format("LVGL's {:s} widget.", element);
					}
					writeElement(name, doc, {}, chainOf(element));
				}
			}

			/** Each component, with its props and what it takes from what its view extends */
			void writeComponents()
			{
				for (const auto& component : m_reference.components)
				{
					std::string doc = fmt::format("Component from {:s}", component.source);
					if (!component.presenter.empty())
					{
						doc += fmt::format(", with presenter {:s}", component.presenter);
					}
					writeElement(component.name, doc + ".", {}, chainOf(component.name));
				}
			}

			/** An attribute to declare */
			struct Attribute
			{
				std::string name;		 // As written
				std::string description; // Its documentation
				std::string type;		 // Its schema type
			};

			/** Declare `attribute`, each line starting with `indent` */
			void writeAttribute(const Attribute& attribute, std::string_view indent)
			{
				fmt::format_to(std::back_inserter(m_out),
							   "{0:s}<xs:attribute name=\"{1:s}\" type=\"{2:s}\">\n"
							   "{0:s}\t<xs:annotation><xs:documentation>{3:s}</xs:documentation></xs:annotation>\n"
							   "{0:s}</xs:attribute>\n",
							   indent,
							   escape(attribute.name),
							   attribute.type,
							   escape(attribute.description));
			}

			/**
			 * Element `name`, documented with `doc`, with `attributes`, then the attribute groups named in `groups`,
			 * and any content and further attributes. A group naming an attribute already declared would declare it
			 * twice, which a schema forbids, so such a group's other attributes are written out instead.
			 */
			void writeElement(std::string_view name,
							  std::string_view doc,
							  const std::vector<Attribute>& attributes,
							  const std::vector<std::string>& groups = {})
			{
				if (!m_declared.insert(std::string(name)).second)
				{
					return;
				}
				fmt::format_to(std::back_inserter(m_out),
							   "\t<xs:element name=\"{:s}\">\n"
							   "\t\t<xs:annotation><xs:documentation>{:s}</xs:documentation></xs:annotation>\n"
							   "\t\t<xs:complexType>\n\t\t\t<xs:group ref=\"anyContent\"/>\n",
							   escape(name),
							   escape(doc));
				std::set<std::string> seen;
				for (const auto& attribute : attributes)
				{
					if (seen.insert(attribute.name).second)
					{
						writeAttribute(attribute, "\t\t\t");
					}
				}
				for (const auto& group : groups)
				{
					const auto found = m_groups.find(group);
					if (found == m_groups.end() || found->second.empty())
					{
						continue;
					}
					const std::vector<Attribute>& groupAttributes = found->second;
					const bool clashes =
						std::any_of(groupAttributes.begin(),
									groupAttributes.end(),
									[&](const Attribute& attribute) { return seen.contains(attribute.name); });
					if (!clashes)
					{
						fmt::format_to(
							std::back_inserter(m_out), "\t\t\t<xs:attributeGroup ref=\"{:s}\"/>\n", groupName(group));
					}
					for (const auto& attribute : groupAttributes)
					{
						if (seen.insert(attribute.name).second && clashes)
						{
							writeAttribute(attribute, "\t\t\t");
						}
					}
				}
				m_out += "\t\t\t<xs:anyAttribute processContents=\"lax\"/>\n\t\t</xs:complexType>\n\t</xs:element>\n";
			}

			const Reference& m_reference;							  // What to describe
			const LvglApi& m_lvgl;									  // The engine's own elements
			std::map<std::string, const WidgetSpec*> m_widgets;		  // DuetScreen's widgets, by name
			std::map<std::string, const LvglElement*> m_lvglElements; // The engine's elements, by name
			std::map<std::string, const Component*> m_components;	  // Components, by name
			std::set<std::string> m_enums;							  // The engine's enum types
			std::map<std::string, std::vector<Attribute>> m_groups;	  // Attributes each name takes itself
			std::set<std::string> m_declared;						  // Elements already declared
			std::string m_out;										  // The schema so far
		};
	} // namespace

	std::string toXsd(const Reference& reference, const LvglApi& lvgl)
	{
		return Writer(reference, lvgl).write();
	}
} // namespace UI::Xml::Api

/*
 * ApiXsd.cpp
 */

#include "ApiXsd.h"
#include <fmt/format.h>
#include <iterator>
#include <map>
#include <set>

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

		/** Writes the schema */
		class Writer
		{
		  public:
			/** Write the schema for `reference` */
			explicit Writer(const Reference& reference)
				: m_reference(reference)
			{
			}

			/** The schema */
			std::string write()
			{
				m_out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
						 "<!-- Generated from the code by tests/src/test_cases/UI/test_xml_ui_api.cpp, like "
						 "XML_UI_API.md. Don't edit it. -->\n"
						 "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n";
				writeNameTypes();
				writeOpenContent();
				writeStructure();
				writeWidgets();
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
				std::set<std::string> views = components;
				views.insert(m_reference.nativeViews.begin(), m_reference.nativeViews.end());
				std::set<std::string> widgets = components;
				for (const auto& widget : m_reference.widgets)
				{
					widgets.insert(widget.name);
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
			}

			/** Type `name`: any string, offering `names` */
			void writeNameType(std::string_view name, const std::set<std::string>& names)
			{
				fmt::format_to(std::back_inserter(m_out),
							   "\t<xs:simpleType name=\"{0:s}.names\">\n\t\t<xs:restriction base=\"xs:string\">\n",
							   name);
				for (const auto& value : names)
				{
					fmt::format_to(
						std::back_inserter(m_out), "\t\t\t<xs:enumeration value=\"{:s}\"/>\n", escape(value));
				}
				fmt::format_to(
					std::back_inserter(m_out),
					"\t\t</xs:restriction>\n\t</xs:simpleType>\n"
					"\t<xs:simpleType name=\"{0:s}\">\n\t\t<xs:union memberTypes=\"{0:s}.names xs:string\"/>\n"
					"\t</xs:simpleType>\n",
					name);
			}

			/** The content every widget may have: any elements, any attributes (its base widget's) */
			void writeOpenContent()
			{
				m_out += "\t<xs:group name=\"anyContent\">\n\t\t<xs:sequence>\n"
						 "\t\t\t<xs:any minOccurs=\"0\" maxOccurs=\"unbounded\" processContents=\"lax\"/>\n"
						 "\t\t</xs:sequence>\n\t</xs:group>\n";
			}

			/** `<component>`, `<api>`, `<prop>`, `<view>` and `<style>` */
			void writeStructure()
			{
				writeElement(
					"component",
					"A component: an `<api>` of props, `<consts>`, `<styles>`, `<subjects>`, and its `<view>`.",
					{{"presenter", "The presenter that gives its view names and handles its actions", "presenter"}});
				writeElement("api", "The props the component takes", {});
				writeElement("prop",
							 "A prop: `<my_component title=\"…\"/>` sets `$title` inside the component",
							 {{"name", "Its name", "xs:string"},
							  {"type", "Its type, e.g. string", "xs:string"},
							  {"default", "Its value when not given", "xs:string"}});
				writeElement("view",
							 "The component's root widget. Takes every attribute of the widget it extends.",
							 {{"extends", "The widget, or component, it is (default lv_obj)", "widget"}});
				writeElement(
					"style",
					"Inside a widget: add the style called `name`. Inside `<styles>`: define a style.",
					{{"name", "The style", "style"}, {"selector", "The part and state it applies to", "xs:string"}});
			}

			/** The DuetScreen widgets, `<action>` and the other elements with specs */
			void writeWidgets()
			{
				for (const auto& widget : m_reference.widgets)
				{
					// `lv_obj-x` is a child element of any widget, written <x>
					std::string name = widget.name;
					if (name.starts_with("lv_obj-"))
					{
						name = name.substr(7);
					}
					std::string doc = widget.description;
					if (!widget.extends.empty())
					{
						doc += fmt::format(" Also takes every {:s} attribute.", widget.extends);
					}
					std::vector<Attribute> attributes;
					for (const WidgetSpec* spec = &widget; spec != nullptr; spec = specNamed(spec->extends))
					{
						for (const auto& attribute : spec->attributes)
						{
							attributes.push_back({attribute.name, attribute.description, typeOf(attribute.kind)});
						}
					}
					writeElement(name, doc, attributes);
				}
			}

			/** Each component, with its props */
			void writeComponents()
			{
				for (const auto& component : m_reference.components)
				{
					std::string doc = fmt::format("Component from {:s}", component.source);
					if (!component.presenter.empty())
					{
						doc += fmt::format(", with presenter {:s}", component.presenter);
					}
					std::vector<Attribute> attributes;
					for (const auto& prop : component.props)
					{
						attributes.push_back(
							{prop.name, fmt::format("Prop `{:s}` ({:s})", prop.name, prop.type), "xs:string"});
					}
					writeElement(component.name, doc + ".", attributes);
				}
			}

			/** An attribute to declare */
			struct Attribute
			{
				std::string name;		 // As written
				std::string description; // Its documentation
				std::string type;		 // Its schema type
			};

			/** Element `name`, documented with `doc`, with `attributes` and any content and further attributes */
			void writeElement(std::string_view name, std::string_view doc, const std::vector<Attribute>& attributes)
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
					if (!seen.insert(attribute.name).second)
					{
						continue;
					}
					fmt::format_to(std::back_inserter(m_out),
								   "\t\t\t<xs:attribute name=\"{:s}\" type=\"{:s}\">\n"
								   "\t\t\t\t<xs:annotation><xs:documentation>{:s}</xs:documentation></xs:annotation>\n"
								   "\t\t\t</xs:attribute>\n",
								   escape(attribute.name),
								   attribute.type,
								   escape(attribute.description));
				}
				m_out += "\t\t\t<xs:anyAttribute processContents=\"lax\"/>\n\t\t</xs:complexType>\n\t</xs:element>\n";
			}

			/** The spec of widget `name`, or nullptr */
			const WidgetSpec* specNamed(std::string_view name) const
			{
				for (const auto& widget : m_reference.widgets)
				{
					if (widget.name == name)
					{
						return &widget;
					}
				}
				return nullptr;
			}

			const Reference& m_reference;	  // What to describe
			std::set<std::string> m_declared; // Elements already declared
			std::string m_out;				  // The schema so far
		};
	} // namespace

	std::string toXsd(const Reference& reference)
	{
		return Writer(reference).write();
	}
} // namespace UI::Xml::Api

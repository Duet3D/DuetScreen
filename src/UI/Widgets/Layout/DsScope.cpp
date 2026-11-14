/*
 * DsScope.cpp
 */

#include "DsScope.h"
#include "Debug.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Binding/Names.h"
#include "UI/Xml/Om/OmBindings.h"
#include "UI/Xml/Om/OmName.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsScope::DsScope(lv_obj_t* container,
					 OmBindings& bindings,
					 OmBoundPath element,
					 const std::string& item,
					 lv_xml_subject_ctx_t* parentContext,
					 const ItemAttributes& itemAttributes)
		: m_scope(std::make_unique<Scope>(parentContext, Scope::Kind::row))
	{
		m_names.setItem(bindings, std::move(element));
		m_scope->setResolver(&m_names, std::string(Names::ROW));
		const std::string name = "item";
		const auto attrs = itemAttributes.attrsFor(name);
		m_obj = static_cast<lv_obj_t*>(lv_xml_create_with_ctx(
			container, item.c_str(), const_cast<const char**>(attrs.data()), m_scope->context()));
		if (m_obj == nullptr)
		{
			LOG_ERROR("<ds_scope>: couldn't create item component '{:s}'", item);
		}
	}

	DsScope::~DsScope()
	{
		// LVGL deletes the container's children after this, its delete event, but the scope may keep subjects the
		// item is bound to (see Scope::keep()): the item goes first
		if (m_obj)
		{
			lv_obj_delete(m_obj);
		}
	}

	void DsScope::registerXml()
	{
		Widgets::add(
			{.name = "ds_scope",
			 .extends = "lv_obj",
			 .description =
				 "Builds one component in a scope where names are relative to an object model element, as in a row of "
				 "an object model list: `current`, `model.pid.p`, `heaters` as a list. When an index of the path is a "
				 "subject's value, the names follow the element it points at, without rebuilding.",
			 .example =
				 R"(<ds_scope path="om.tools[{om.state.currentTool}]" item="dashboard_current_tool_card" width="300"/>)",
			 .attributes = {{"path",
							 "The element (required): an `om.` path, whose indices may be `{name}`, a subject's value",
							 AttributeKind::Subject},
							{"item", "The component built (required)", AttributeKind::Component},
							{"item.<attribute>",
							 "Given to the item as if written where it is used: a prop of the item component, or an "
							 "attribute of its root. Its names are looked up in the item's scope.",
							 AttributeKind::Items}}},
			xmlCreate,
			xmlApply);
	}

	void* DsScope::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		return lv_obj_create(parentOf(state));
	}

	void DsScope::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		const char* pathName = lv_xml_get_value_of(attrs, "path");
		const char* item = lv_xml_get_value_of(attrs, "item");
		if (pathName == nullptr || item == nullptr)
		{
			LOG_WARN("<ds_scope> needs both path= and item=");
			return;
		}
		OmBindings* bindings = OmBindings::current();
		const auto name = OmName::parse(pathName);
		const auto path = name ? OmTemplate::parse(name->path) : std::nullopt;
		if (bindings == nullptr || !name || !name->absolute || !name->converter.empty() || !path)
		{
			LOG_WARN("<ds_scope path=\"{:s}\">: not an `om.` path", pathName);
			return;
		}

		lv_xml_subject_ctx_t* context = lv_xml_subject_ctx_get_active();
		std::string missing;
		auto element = OmBoundPath::bind(
			*path, [context](std::string_view index) { return lookupSubject(context, index); }, missing);
		if (!element)
		{
			LOG_WARN("<ds_scope path=\"{:s}\">: no subject '{:s}' for its index", pathName, missing);
			return;
		}
		lv_obj_t* container = itemOf(state);
		deleteWith(container,
				   new DsScope(container, *bindings, std::move(*element), item, context, ItemAttributes::from(attrs)));
	}
} // namespace UI::Xml

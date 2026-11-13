/*
 * DsToggle.cpp
 */

#include "DsToggle.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsToggle::DsToggle(const std::string& name, lv_obj_t* parent)
		: DsToggle(name, LvObjRef(parent))
	{
	}

	DsToggle::DsToggle(const std::string& name, LvObjRef&& parent)
		: UI_SETTINGS_TOGGLE_WIDGET(name, parent)
	{
	}

	void DsToggle::registerXml()
	{
		lv_xml_register_widget("ds_toggle", xmlCreate, xmlApply);
	}

	void* DsToggle::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* toggle = new DsToggle(attrOr(attrs, "name", "toggle"), parentOf(state));
		deleteWith(toggle->getRootPtr(), toggle);
		return toggle->getRootPtr();
	}

	void DsToggle::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
	}
} // namespace UI::Xml

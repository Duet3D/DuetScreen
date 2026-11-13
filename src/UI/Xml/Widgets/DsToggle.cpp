/*
 * DsToggle.cpp
 */

#include "DsToggle.h"
#include "WidgetSupport.h"
#include "Widgets.h"
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
		Widgets::add({.name = "ds_toggle",
					  .extends = "lv_obj",
					  .description = "An on/off setting, drawn as the build's toggle (a check box or a switch), so "
									 "every setting looks the same. Raises `value_changed` when it is changed; the "
									 "action's value is 1 when turned on and 0 when off.",
					  .example = R"(<ds_toggle>
	<bind_state_if_eq subject="confirmations" state="checked" ref_value="1"/>
	<action trigger="value_changed" name="settings.confirmations"/>
</ds_toggle>)"},
					 xmlCreate,
					 xmlApply);
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

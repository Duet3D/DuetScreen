/*
 * DsPasswordToggle.cpp
 */

#include "DsPasswordToggle.h"
#include "Debug.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsPasswordToggle::DsPasswordToggle(const std::string& name, lv_obj_t* parent)
		: Button(name, parent, LV_SYMBOL_EYE_OPEN)
	{
		setCheckable(true);
		// Tapping it leaves the box focused, so the keyboard stays up
		lv_obj_remove_flag(getRootPtr(), LV_OBJ_FLAG_CLICK_FOCUSABLE);
		lv_obj_add_event_cb(getRootPtr(), toggledCb, LV_EVENT_VALUE_CHANGED, this);
	}

	void DsPasswordToggle::registerXml()
	{
		Widgets::add(
			{.name = "ds_password_toggle",
			 .extends = "lv_obj",
			 .description = "An eye button that shows what is typed in a password box while it is checked, and hides "
							"it again when unchecked. Tapping it doesn't take the focus from the box, so the keyboard "
							"stays up.",
			 .example = R"(<lv_obj width="100%" height="content" flex_flow="row" style_pad_all="0">
	<ds_textarea name="password" one_line="true" password_mode="true" flex_grow="1"/>
	<ds_password_toggle textarea="password"/>
</lv_obj>)",
			 .attributes = {{"textarea",
							 "Name of the password box: the nearest `ds_textarea` of that name around the button, "
							 "e.g. its sibling"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsPasswordToggle::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* toggle = new DsPasswordToggle(attrOr(attrs, "name", "show_password"), parentOf(state));
		deleteWith(toggle->getRootPtr(), toggle);
		return toggle->getRootPtr();
	}

	void DsPasswordToggle::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		auto& toggle = static_cast<DsPasswordToggle&>(*LvObj::fromPtr(itemOf(state)));
		if (const char* textarea = lv_xml_get_value_of(attrs, "textarea"))
		{
			toggle.m_textarea = textarea;
		}
		else
		{
			LOG_WARN("<ds_password_toggle> needs a textarea=");
		}
	}

	lv_obj_t* DsPasswordToggle::findTextarea() const
	{
		if (m_textarea.empty())
		{
			return nullptr;
		}
		for (lv_obj_t* scope = lv_obj_get_parent(getRootPtr()); scope != nullptr; scope = lv_obj_get_parent(scope))
		{
			lv_obj_t* found = lv_obj_find_by_name(scope, m_textarea.c_str());
			if (found != nullptr && lv_obj_check_type(found, &lv_textarea_class))
			{
				return found;
			}
		}
		return nullptr;
	}

	void DsPasswordToggle::toggledCb(lv_event_t* e)
	{
		auto* toggle = static_cast<DsPasswordToggle*>(lv_event_get_user_data(e));
		lv_obj_t* textarea = toggle->findTextarea();
		if (textarea == nullptr)
		{
			LOG_WARN("<ds_password_toggle>: no text box called '{:s}'", toggle->m_textarea);
			return;
		}
		lv_textarea_set_password_mode(textarea, !lv_obj_has_state(toggle->getRootPtr(), LV_STATE_CHECKED));
	}
} // namespace UI::Xml

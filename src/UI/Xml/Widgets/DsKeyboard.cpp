/*
 * DsKeyboard.cpp
 */

#include "DsKeyboard.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsKeyboard::DsKeyboard(const std::string& name, lv_obj_t* parent)
		: DsKeyboard(name, LvObjRef(parent))
	{
	}

	DsKeyboard::DsKeyboard(const std::string& name, LvObjRef&& parent)
		: LvKeyboard(name, parent)
	{
	}

	void DsKeyboard::registerXml()
	{
		Widgets::add(
			{.name = "ds_keyboard",
			 .extends = "lv_obj",
			 .description = "An on-screen keyboard with DuetScreen's key layouts. A `<ds_textarea>` brings it up and "
							"types into it.",
			 .example =
				 R"(<ds_keyboard name="console_keyboard" mode="text_upper" width="100%" height="40%" hidden="true"/>)",
			 .attributes = {{"mode", "Key layout: `text` (default), `text_upper` or `number`"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsKeyboard::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* keyboard = new DsKeyboard(attrOr(attrs, "name", "keyboard"), parentOf(state));
		deleteWith(keyboard->getRootPtr(), keyboard);
		return keyboard->getRootPtr();
	}

	void DsKeyboard::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
		const std::string_view mode = attrOr(attrs, "mode", "text");
		lv_keyboard_set_mode(itemOf(state),
							 mode == "number"		? LV_KEYBOARD_MODE_NUMBER
							 : mode == "text_upper" ? LV_KEYBOARD_MODE_TEXT_UPPER
													: LV_KEYBOARD_MODE_TEXT_LOWER);
	}
} // namespace UI::Xml

/*
 * DsKeyboard.cpp
 */

#include "DsKeyboard.h"
#include "WidgetSupport.h"
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
		lv_xml_register_widget("ds_keyboard", xmlCreate, xmlApply);
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

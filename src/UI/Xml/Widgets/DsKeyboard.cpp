/*
 * DsKeyboard.cpp
 */

#include "DsKeyboard.h"
#include "Debug.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		constexpr const char* SIGN_KEY = "+/-"; // The number pad's sign key

		/** The key layout called `mode` */
		lv_keyboard_mode_t keyboardMode(std::string_view mode)
		{
			if (mode == "number")
			{
				return LV_KEYBOARD_MODE_NUMBER;
			}
			if (mode == "numberpad")
			{
				return LvKeyboard::NUMBER_PAD_MODE;
			}
			return mode == "text_upper" ? LV_KEYBOARD_MODE_TEXT_UPPER : LV_KEYBOARD_MODE_TEXT_LOWER;
		}

		/** The nearest text box called `name` around `obj`, or nullptr */
		lv_obj_t* findTextarea(lv_obj_t* obj, const char* name)
		{
			for (lv_obj_t* scope = lv_obj_get_parent(obj); scope != nullptr; scope = lv_obj_get_parent(scope))
			{
				lv_obj_t* found = lv_obj_find_by_name(scope, name);
				if (found != nullptr && lv_obj_check_type(found, &lv_textarea_class))
				{
					return found;
				}
			}
			return nullptr;
		}
	} // namespace

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
							"types into it, so it is usually written `hidden=\"true\"`; or, with `textarea`, it always "
							"types into one box, as the number pad's keys do. Its OK key raises `ready`.",
			 .example = R"(<ds_keyboard name="console_keyboard" mode="text_upper" width="100%" height="200"/>)",
			 .attributes = {{"mode", "Key layout: `text` (default), `text_upper`, `number` or `numberpad`"},
							{"textarea",
							 "Always type into the text box of this name: the nearest one around the keyboard, written "
							 "before it"},
							{"bind_ok_enabled",
							 "Follow an int subject: the OK key works only while it is non-zero",
							 AttributeKind::Subject},
							{"bind_sign_enabled",
							 "Follow an int subject: the `numberpad` layout's +/- key works only while it is non-zero",
							 AttributeKind::Subject}}},
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

		lv_obj_t* root = itemOf(state);
		auto& keyboard = static_cast<DsKeyboard&>(*LvObj::fromPtr(root));
		lv_keyboard_set_mode(root, keyboardMode(attrOr(attrs, "mode", "text")));

		if (const char* name = lv_xml_get_value_of(attrs, "textarea"))
		{
			if (lv_obj_t* textarea = findTextarea(root, name))
			{
				lv_keyboard_set_textarea(root, textarea);
			}
			else
			{
				LOG_WARN("<ds_keyboard textarea=\"{:s}\">: no text box of that name before it", name);
			}
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_ok_enabled"))
		{
			lv_subject_add_observer_obj(subject, okObserverCb, root, &keyboard);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_sign_enabled"))
		{
			lv_subject_add_observer_obj(subject, signObserverCb, root, &keyboard);
		}
	}

	void DsKeyboard::okObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsKeyboard*>(lv_observer_get_user_data(observer))
			->enableKey(LV_SYMBOL_OK, lv_subject_get_int(subject) != 0);
	}

	void DsKeyboard::signObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsKeyboard*>(lv_observer_get_user_data(observer))
			->enableKey(SIGN_KEY, lv_subject_get_int(subject) != 0);
	}

	void DsKeyboard::enableKey(const char* text, bool enabled)
	{
		lv_obj_t* root = getRootPtr();
		const char* const* map = lv_buttonmatrix_get_map(root);
		// Buttons are numbered without the "\n" row breaks
		uint32_t button = 0;
		for (size_t i = 0; map[i][0] != '\0'; i++)
		{
			if (lv_streq(map[i], "\n"))
			{
				continue;
			}
			if (lv_streq(map[i], text))
			{
				if (enabled)
				{
					lv_buttonmatrix_clear_button_ctrl(root, button, LV_BUTTONMATRIX_CTRL_DISABLED);
				}
				else
				{
					lv_buttonmatrix_set_button_ctrl(root, button, LV_BUTTONMATRIX_CTRL_DISABLED);
				}
				return;
			}
			button++;
		}
	}
} // namespace UI::Xml

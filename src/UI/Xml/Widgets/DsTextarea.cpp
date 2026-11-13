/*
 * DsTextarea.cpp
 */

#include "DsTextarea.h"
#include "UI/Core/Navigation.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "i18n/i18n.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsTextarea::DsTextarea(const std::string& name, lv_obj_t* parent)
		: DsTextarea(name, LvObjRef(parent))
	{
	}

	namespace
	{
		/** The keyboard layout called `mode` */
		lv_keyboard_mode_t keyboardMode(std::string_view mode)
		{
			if (mode == "number")
			{
				return LV_KEYBOARD_MODE_NUMBER;
			}
			return mode == "text_upper" ? LV_KEYBOARD_MODE_TEXT_UPPER : LV_KEYBOARD_MODE_TEXT_LOWER;
		}
	} // namespace

	DsTextarea::DsTextarea(const std::string& name, LvObjRef&& parent)
		: LvTextarea(name, parent)
	{
		for (lv_event_code_t code : {LV_EVENT_FOCUSED, LV_EVENT_DEFOCUSED, LV_EVENT_READY, LV_EVENT_CANCEL})
		{
			lv_obj_add_event_cb(getRootPtr(), keyboardEventCb, code, this);
		}
	}

	void DsTextarea::registerXml()
	{
		Widgets::add(
			{.name = "ds_textarea",
			 .extends = "lv_obj",
			 .description =
				 "A text box that brings up an on-screen keyboard while it is being edited. The keyboard's own close "
				 "key always puts it away. Raises `focused`, `defocused`, `ready` and `cancel` as the edit starts and "
				 "ends, and `value_changed` on every edit; handlers read the text with ActionArgs::text().",
			 .example =
				 R"(<ds_textarea name="input" one_line="true" bind_text="alert.input" bind_keyboard_mode="alert.keyboard_mode"
			 width="300">
	<action trigger="value_changed" name="alert.input"/>
</ds_textarea>)",
			 .attributes =
				 {{"bind_text",
				   "Follow a text subject: the box shows its value whenever it changes to something other than what is "
				   "in the box, so a presenter that keeps the subject equal to the edits can replace the text at any "
				   "time",
				   AttributeKind::Subject},
				  {"bind_accepted_chars",
				   "Follow a text subject for the characters that may be typed; empty for any",
				   AttributeKind::Subject},
				  {"keyboard_mode", "Keyboard layout to show: `text` (default), `text_upper` or `number`"},
				  {"bind_keyboard_mode",
				   "Follow a text subject for the keyboard layout, as `keyboard_mode`",
				   AttributeKind::Subject},
				  {"keyboard",
				   "The keyboard to use: the nearest lv_keyboard of that name around the box (e.g. one in the same "
				   "view), else the view of that name (default `keyboard`, the shared one); `none` for a box edited "
				   "another way, such as a number pad"},
				  {"hide_keyboard_on_ready", "Put the keyboard away when its OK key is pressed (default true)"},
				  {"hide_keyboard_on_defocus", "Put the keyboard away when the box loses focus (default true)"},
				  {"one_line", "\"true\" for a single line of text"},
				  {"password_mode", "\"true\" to show dots for the characters, e.g. for a password"},
				  {"placeholder", "Text shown while the box is empty"},
				  {"placeholder_tag",
				   "Translation key for the placeholder, instead of `placeholder`",
				   AttributeKind::Translation}},
			 .exampleSubjects = {{"alert.input", "string", "M115"}, {"alert.keyboard_mode", "string", "text"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsTextarea::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* textarea = new DsTextarea(attrOr(attrs, "name", "textarea"), parentOf(state));
		deleteWith(textarea->getRootPtr(), textarea);
		return textarea->getRootPtr();
	}

	void DsTextarea::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto& textarea = static_cast<DsTextarea&>(*LvObj::fromPtr(root));
		if (const char* oneLine = lv_xml_get_value_of(attrs, "one_line"))
		{
			lv_textarea_set_one_line(root, lv_xml_to_bool(oneLine));
		}
		if (const char* password = lv_xml_get_value_of(attrs, "password_mode"))
		{
			lv_textarea_set_password_mode(root, lv_xml_to_bool(password));
		}
		if (const char* tag = lv_xml_get_value_of(attrs, "placeholder_tag"))
		{
			lv_textarea_set_placeholder_text(root, _(tag).c_str());
		}
		else if (const char* placeholder = lv_xml_get_value_of(attrs, "placeholder"))
		{
			lv_textarea_set_placeholder_text(root, placeholder);
		}
		textarea.m_mode = keyboardMode(attrOr(attrs, "keyboard_mode", "text"));
		textarea.m_keyboard = attrOr(attrs, "keyboard", "keyboard");
		textarea.m_hideOnReady = lv_xml_to_bool(attrOr(attrs, "hide_keyboard_on_ready", "true"));
		textarea.m_hideOnDefocus = lv_xml_to_bool(attrOr(attrs, "hide_keyboard_on_defocus", "true"));

		if (lv_subject_t* subject = findSubject(state, attrs, "bind_accepted_chars"))
		{
			lv_subject_add_observer_obj(subject, acceptedObserverCb, root, &textarea);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_keyboard_mode"))
		{
			lv_subject_add_observer_obj(subject, keyboardModeObserverCb, root, &textarea);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_text"))
		{
			lv_subject_add_observer_obj(subject, textObserverCb, root, &textarea);
		}
	}

	void DsTextarea::textObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		lv_obj_t* textarea = lv_observer_get_target_obj(observer);
		const char* text = lv_subject_get_string(subject);
		// Already showing it, e.g. the edit that the presenter is echoing: don't move the cursor
		if (std::string_view(lv_textarea_get_text(textarea)) != text)
		{
			lv_textarea_set_text(textarea, text);
		}
	}

	void DsTextarea::acceptedObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* textarea = static_cast<DsTextarea*>(lv_observer_get_user_data(observer));
		textarea->m_accepted = lv_subject_get_string(subject);
		lv_textarea_set_accepted_chars(textarea->getRootPtr(),
									   textarea->m_accepted.empty() ? nullptr : textarea->m_accepted.c_str());
	}

	void DsTextarea::keyboardModeObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		static_cast<DsTextarea*>(lv_observer_get_user_data(observer))->m_mode =
			keyboardMode(lv_subject_get_string(subject));
	}

	lv_obj_t* DsTextarea::findKeyboard() const
	{
		if (m_keyboard == "none")
		{
			return nullptr;
		}
		// The nearest one of that name: in the same view before any other
		for (lv_obj_t* scope = lv_obj_get_parent(getRootPtr()); scope != nullptr; scope = lv_obj_get_parent(scope))
		{
			lv_obj_t* found = lv_obj_find_by_name(scope, m_keyboard.c_str());
			if (found != nullptr && lv_obj_check_type(found, &lv_keyboard_class))
			{
				return found;
			}
		}
		LvObj* view = findView(m_keyboard);
		return view != nullptr && lv_obj_check_type(view->getRootPtr(), &lv_keyboard_class) ? view->getRootPtr()
																							: nullptr;
	}

	void DsTextarea::showKeyboard(lv_obj_t* keyboard, bool show)
	{
		if (show)
		{
			lv_keyboard_set_mode(keyboard, m_mode);
			lv_keyboard_set_textarea(keyboard, getRootPtr());
			lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
			lv_obj_move_foreground(keyboard);
			return;
		}
		lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
	}

	void DsTextarea::keyboardEventCb(lv_event_t* e)
	{
		auto* textarea = static_cast<DsTextarea*>(lv_event_get_user_data(e));
		lv_obj_t* keyboard = textarea->findKeyboard();
		if (keyboard == nullptr)
		{
			return;
		}

		switch (lv_event_get_code(e))
		{
		case LV_EVENT_FOCUSED:
			textarea->showKeyboard(keyboard, true);
			break;
		case LV_EVENT_DEFOCUSED:
			if (textarea->m_hideOnDefocus)
			{
				lv_keyboard_set_textarea(keyboard, nullptr);
				textarea->showKeyboard(keyboard, false);
			}
			break;
		case LV_EVENT_READY:
			if (textarea->m_hideOnReady)
			{
				textarea->showKeyboard(keyboard, false);
			}
			break;
		case LV_EVENT_CANCEL:
			textarea->showKeyboard(keyboard, false);
			break;
		default:
			break;
		}
	}
} // namespace UI::Xml

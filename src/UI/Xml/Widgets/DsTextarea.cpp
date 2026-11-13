/*
 * DsTextarea.cpp
 */

#include "DsTextarea.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Core/Navigation.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsTextarea::DsTextarea(const std::string& name, lv_obj_t* parent)
		: DsTextarea(name, LvObjRef(parent))
	{
	}

	DsTextarea::DsTextarea(const std::string& name, LvObjRef&& parent)
		: LvTextarea(name, parent)
	{
		lv_obj_add_event_cb(getRootPtr(), keyboardEventCb, LV_EVENT_FOCUSED, this);
		lv_obj_add_event_cb(getRootPtr(), keyboardEventCb, LV_EVENT_DEFOCUSED, this);
		lv_obj_add_event_cb(getRootPtr(), keyboardEventCb, LV_EVENT_READY, this);
	}

	void DsTextarea::registerXml()
	{
		lv_xml_register_widget("ds_textarea", xmlCreate, xmlApply);
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
		lv_textarea_set_text(lv_observer_get_target_obj(observer), lv_subject_get_string(subject));
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
		auto* textarea = static_cast<DsTextarea*>(lv_observer_get_user_data(observer));
		textarea->m_numberKeyboard = std::string_view(lv_subject_get_string(subject)) == "number";
	}

	void DsTextarea::keyboardEventCb(lv_event_t* e)
	{
		auto* textarea = static_cast<DsTextarea*>(lv_event_get_user_data(e));
		auto* keyboard = findView<LvKeyboard>("keyboard");
		if (keyboard == nullptr)
		{
			return;
		}

		switch (lv_event_get_code(e))
		{
		case LV_EVENT_FOCUSED:
			keyboard->setMode(textarea->m_numberKeyboard ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
			keyboard->setTextarea(textarea);
			keyboard->setVisible(true, true);
			break;
		case LV_EVENT_DEFOCUSED:
			keyboard->setTextarea(nullptr);
			keyboard->setVisible(false);
			break;
		case LV_EVENT_READY:
			keyboard->setVisible(false);
			break;
		default:
			break;
		}
	}
} // namespace UI::Xml

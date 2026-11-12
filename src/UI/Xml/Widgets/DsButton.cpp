/*
 * DsButton.cpp
 */

#include "DsButton.h"
#include "UI/Components/LVGL/LvAnim.h"
#include "WidgetSupport.h"
#include "i18n/i18n.h"
#include "src/xml/lv_xml_base_types.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsButton::DsButton(const std::string& name, lv_obj_t* parent, std::string_view text)
		: Button(name, parent, text)
	{
	}

	void DsButton::registerXml()
	{
		lv_xml_register_widget("ds_button", xmlCreate, xmlApply);
	}

	void* DsButton::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		const char* tag = lv_xml_get_value_of(attrs, "translation_tag");
		const std::string text = tag ? _(tag) : attrOr(attrs, "text", "");
		auto* button = new DsButton(attrOr(attrs, "name", "button"), parentOf(state), text);
		deleteWith(button->getRootPtr(), button);
		return button->getRootPtr();
	}

	void DsButton::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		lv_obj_t* root = itemOf(state);
		auto* button = static_cast<DsButton*>(LvObj::fromPtr(root));
		const char* tag = lv_xml_get_value_of(attrs, "translation_tag");

		for (int i = 0; attrs[i]; i += 2)
		{
			const char* name = attrs[i];
			const char* value = attrs[i + 1];
			if (lv_streq(name, "icon"))
			{
				button->setIcon(value);
			}
			else if (lv_streq(name, "checkable"))
			{
				button->setCheckable(lv_xml_to_bool(value));
			}
			else if (lv_streq(name, "text") && tag == nullptr)
			{
				button->setText(value);
			}
			else if (lv_streq(name, "long_mode"))
			{
				button->getLabel().setLongMode(lv_xml_label_long_mode_to_enum(value));
			}
		}

		if (tag != nullptr)
		{
			if (button->m_translationTag.empty())
			{
				lv_obj_add_event_cb(root, languageChangedCb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, button);
			}
			button->m_translationTag = tag;
		}

		if (lv_subject_t* subject = findSubject(state, attrs, "flash_on"))
		{
			lv_subject_add_observer_obj(subject, flashObserverCb, root, button);
		}

		if (lv_subject_t* subject = findSubject(state, attrs, "bind_text"))
		{
			lv_subject_add_observer_obj(
				subject,
				[](lv_observer_t* observer, lv_subject_t* s)
				{ static_cast<DsButton*>(lv_observer_get_user_data(observer))->setText(lv_subject_get_string(s)); },
				root,
				button);
		}
	}

	void DsButton::flashObserverCb(lv_observer_t* observer, lv_subject_t*)
	{
		auto* button = static_cast<DsButton*>(lv_observer_get_user_data(observer));
		// Adding an observer reports the current value at once: that's not a change
		if (!button->m_flashArmed)
		{
			button->m_flashArmed = true;
			return;
		}
		button->flash();
	}

	void DsButton::flash()
	{
		ZoneScoped;
		LvAnim anim;
		anim.setDuration(500);
		anim.setValues(0, 100);
		anim.setExecCb([](void* var, int32_t value)
					   { static_cast<DsButton*>(var)->setStyleRecolorOpa(static_cast<lv_opa_t>(value)); });
		anim.setCompletedCb(
			[](lv_anim_t* a)
			{
				auto* button = static_cast<DsButton*>(a->var);
				button->setStyleRecolorOpa(LV_OPA_0);
				button->setState(LV_STATE_USER_1, false);
			});
		anim.setPathCb(lv_anim_path_ease_in_out);
		anim.setRepeatCount(1);
		anim.setVar(this);
		anim.setReverseDuration(200);
		setState(LV_STATE_USER_1, true);
		anim.start();
	}

	void DsButton::languageChangedCb(lv_event_t* e)
	{
		auto* button = static_cast<DsButton*>(lv_event_get_user_data(e));
		button->setText(_(button->m_translationTag));
	}
} // namespace UI::Xml

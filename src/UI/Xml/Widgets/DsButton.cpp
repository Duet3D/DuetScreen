/*
 * DsButton.cpp
 */

#include "DsButton.h"
#include "UI/Components/LVGL/LvAnim.h"
#include "WidgetSupport.h"
#include "Widgets.h"
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
		Widgets::add(
			{.name = "ds_button",
			 .extends = "lv_obj",
			 .description = "A button showing text, an icon or both",
			 .example = R"(<ds_button translation_tag="fan.max" icon="fan.png">
	<action trigger="clicked" name="fans.set_speed" arg="100"/>
</ds_button>)",
			 .attributes =
				 {{"text", "The text shown, if there is no `translation_tag`"},
				  {"translation_tag",
				   "Translation key for the text; follows language changes",
				   AttributeKind::Translation},
				  {"bind_text", "Follow a text or int subject for the text", AttributeKind::Subject},
				  {"bind_text-fmt",
				   "printf format for `bind_text`, e.g. \"%d°C\" (default \"%s\" for text, \"%d\" for an int)"},
				  {"icon", "Icon file from the icon set, e.g. \"fan.png\""},
				  {"checkable", "\"true\" to make the button toggle its `checked` state when clicked"},
				  {"long_mode",
				   "What the label does with text too long for it: wrap, dots, scroll, scroll_circular or clip"},
				  {"flash_on",
				   "Flash the button briefly, in its `user_1` state, whenever this subject changes, e.g. "
				   "`responses.count`; style the flash with `<style … selector=\"user_1\"/>`",
				   AttributeKind::Subject}}},
			xmlCreate,
			xmlApply);
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
			button->m_textFormat = attrOr(attrs, "bind_text-fmt", "");
			lv_subject_add_observer_obj(subject, textObserverCb, root, button);
		}
	}

	void DsButton::textObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* button = static_cast<DsButton*>(lv_observer_get_user_data(observer));
		const char* format = button->m_textFormat.empty() ? nullptr : button->m_textFormat.c_str();
		char text[128];
		switch (subject->type)
		{
		case LV_SUBJECT_TYPE_INT:
			lv_snprintf(text, sizeof(text), format ? format : "%d", lv_subject_get_int(subject));
			break;
		case LV_SUBJECT_TYPE_STRING:
			lv_snprintf(text, sizeof(text), format ? format : "%s", lv_subject_get_string(subject));
			break;
		default:
			LOG_WARN("ds_button bind_text needs a string or integer subject");
			return;
		}
		button->setText(text);
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
		// The animation works on the widget, not this object: LVGL stops it when the widget is deleted
		LvAnim anim;
		anim.setDuration(500);
		anim.setValues(0, 100);
		anim.setExecCb(
			[](void* var, int32_t value)
			{ lv_obj_set_style_recolor_opa(static_cast<lv_obj_t*>(var), static_cast<lv_opa_t>(value), LV_PART_MAIN); });
		anim.setCompletedCb(
			[](lv_anim_t* a)
			{
				auto* obj = static_cast<lv_obj_t*>(a->var);
				lv_obj_set_style_recolor_opa(obj, LV_OPA_0, LV_PART_MAIN);
				lv_obj_remove_state(obj, LV_STATE_USER_1);
			});
		anim.setPathCb(lv_anim_path_ease_in_out);
		anim.setRepeatCount(1);
		anim.setVar(getRootPtr());
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

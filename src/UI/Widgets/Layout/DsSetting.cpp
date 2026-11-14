/*
 * DsSetting.cpp
 */

#include "DsSetting.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Runtime/Translations.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include "utils/utils.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The gap between the title and its badge */
		constexpr lv_coord_t BADGE_GAP = 8;
	} // namespace

	DsSetting::DsSetting(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		addStyle(Themes::getComponentStyles().setting_row);
		addStyle(Themes::getLvglStyles().pressed, LV_STATE_PRESSED);
		setHeight(LV_SIZE_CONTENT);
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		// The text passes taps on to the row
		m_text.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_text.setFlexGrow(1);
		m_text.setStylePad(0);
		m_text.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_text.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		m_text.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		// A badge follows the title (see setBadge())
		m_heading.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_heading.setStylePad(0);
		m_heading.setStylePad(BADGE_GAP, 0, Padding::COLUMN);
		m_heading.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_heading.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_heading.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		m_heading.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_title.setLongMode(LV_LABEL_LONG_MODE_WRAP);
		m_title.addStyle(Themes::getComponentStyles().setting_title);
		m_title.setText("");

		m_badge.addStyle(Themes::getComponentStyles().setting_badge);
		setBadge("");

		m_hint.setWidth(LV_PCT(100));
		m_hint.setLongMode(LV_LABEL_LONG_MODE_WRAP);
		m_hint.addStyle(Themes::getComponentStyles().setting_hint);
		setHint("");

		lv_obj_add_event_cb(getRootPtr(), pressedCb, LV_EVENT_PRESSED, this);
		lv_obj_add_event_cb(getRootPtr(), clickedCb, LV_EVENT_CLICKED, this);
		lv_obj_add_event_cb(getRootPtr(), languageChangedCb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, this);
	}

	void DsSetting::registerXml()
	{
		Widgets::add(
			{.name = "ds_setting",
			 .extends = "lv_obj",
			 .description =
				 "One setting, a row: its title, a hint under it if it has one, then its control, the children written "
				 "inside it, at the end of the row. Tapping the row anywhere works the control as tapping it would, if "
				 "it is a switch, a button or a `<ds_choice>`. In a group, an `lv_obj` with the `setting_group` style "
				 "under a label with the `setting_group_title` style. In a dialog's list, add the `dialog_row` style.",
			 .example = R"(<ds_setting title_tag="tr.settings.show_confirmation_dialogs"
			hint="Ask first before actions that change the machine">
	<ds_toggle>
		<bind_state_if_eq subject="presenter.confirmations" state="checked" ref_value="1"/>
		<action trigger="value_changed" name="presenter.settings.confirmations"/>
	</ds_toggle>
</ds_setting>)",
			 .attributes =
				 {{"title", "The setting's title, if there is no `title_tag` or `bind_title`"},
				  {"title_tag", "Translation key for the title; follows language changes", AttributeKind::Translation},
				  {"bind_title", "Follow a text subject for the title", AttributeKind::Subject},
				  {"hint",
				   "A line under the title saying more about the setting, if there is no `hint_tag` or `bind_hint`"},
				  {"hint_tag", "Translation key for the hint; follows language changes", AttributeKind::Translation},
				  {"badge",
				   "A word after the title saying what sort of setting it is, e.g. \"Advanced\", if there is no "
				   "`badge_tag`"},
				  {"badge_tag", "Translation key for the badge; follows language changes", AttributeKind::Translation},
				  {"bind_hint",
				   "Follow a text subject for the hint, e.g. how something went; no hint while it is empty",
				   AttributeKind::Subject}},
			 .exampleSubjects = {{"presenter.confirmations", "int", "1"}}},
			xmlCreate,
			xmlApply);
	}

	lv_obj_t* DsSetting::control() const
	{
		const uint32_t count = lv_obj_get_child_count(getRootPtr());
		for (uint32_t i = 0; i < count; i++)
		{
			lv_obj_t* child = lv_obj_get_child(getRootPtr(), static_cast<int32_t>(i));
			if (child == m_text.getRootPtr() || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN))
			{
				continue;
			}
			if (lv_obj_has_flag(child, LV_OBJ_FLAG_CHECKABLE) || lv_obj_check_type(child, &lv_button_class))
			{
				return child;
			}
		}
		return nullptr;
	}

	void* DsSetting::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* setting = new DsSetting(attrOr(attrs, "name", "setting"), parentOf(state));
		deleteWith(setting->getRootPtr(), setting);
		return setting->getRootPtr();
	}

	void DsSetting::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		auto* setting = static_cast<DsSetting*>(LvObj::fromPtr(itemOf(state)));
		if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"))
		{
			setting->m_titleTag = tag;
			setting->m_title.setText(Translations::translate(tag));
		}
		else if (const char* title = lv_xml_get_value_of(attrs, "title"))
		{
			setting->m_title.setText(title);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_title"))
		{
			lv_subject_add_observer_obj(subject, titleObserverCb, setting->getRootPtr(), setting);
		}

		if (const char* tag = lv_xml_get_value_of(attrs, "hint_tag"))
		{
			setting->m_hintTag = tag;
			setting->setHint(Translations::translate(tag));
		}
		else if (const char* hint = lv_xml_get_value_of(attrs, "hint"))
		{
			setting->setHint(hint);
		}
		if (const char* tag = lv_xml_get_value_of(attrs, "badge_tag"))
		{
			setting->m_badgeTag = tag;
			setting->setBadge(Translations::translate(tag));
		}
		else if (const char* badge = lv_xml_get_value_of(attrs, "badge"))
		{
			setting->setBadge(badge);
		}
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_hint"))
		{
			lv_subject_add_observer_obj(subject, hintObserverCb, setting->getRootPtr(), setting);
		}
	}

	void DsSetting::hintObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* setting = static_cast<DsSetting*>(lv_observer_get_user_data(observer));
		setting->setHint(lv_subject_get_string(subject));
	}

	void DsSetting::titleObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* setting = static_cast<DsSetting*>(lv_observer_get_user_data(observer));
		setting->m_title.setText(lv_subject_get_string(subject));
	}

	void DsSetting::languageChangedCb(lv_event_t* e)
	{
		auto* setting = static_cast<DsSetting*>(lv_event_get_user_data(e));
		if (!setting->m_titleTag.empty())
		{
			setting->m_title.setText(Translations::translate(setting->m_titleTag));
		}
		if (!setting->m_hintTag.empty())
		{
			setting->setHint(Translations::translate(setting->m_hintTag));
		}
		if (!setting->m_badgeTag.empty())
		{
			setting->setBadge(Translations::translate(setting->m_badgeTag));
		}
	}

	void DsSetting::pressedCb(lv_event_t* e)
	{
		const auto* setting = static_cast<DsSetting*>(lv_event_get_user_data(e));
		if (setting->control() == nullptr)
		{
			lv_obj_remove_state(setting->getRootPtr(), LV_STATE_PRESSED);
		}
	}

	void DsSetting::clickedCb(lv_event_t* e)
	{
		const auto* setting = static_cast<DsSetting*>(lv_event_get_user_data(e));
		lv_obj_t* control = setting->control();
		if (control == nullptr || lv_obj_has_state(control, LV_STATE_DISABLED))
		{
			return;
		}
		if (lv_obj_has_flag(control, LV_OBJ_FLAG_CHECKABLE))
		{
			// As a tap on the control: LVGL turns it over when it is released, then says it changed
			if (lv_obj_has_state(control, LV_STATE_CHECKED))
			{
				lv_obj_remove_state(control, LV_STATE_CHECKED);
			}
			else
			{
				lv_obj_add_state(control, LV_STATE_CHECKED);
			}
			lv_obj_send_event(control, LV_EVENT_VALUE_CHANGED, nullptr);
		}
		else
		{
			lv_obj_send_event(control, LV_EVENT_CLICKED, nullptr);
		}
	}

	void DsSetting::setBadge(std::string_view badge)
	{
		m_badge.setText(utils::toUpper(badge));
		m_badge.setFlag(LV_OBJ_FLAG_HIDDEN, badge.empty());
		// Alone, the title takes the whole line and wraps in it; with a badge, it is as wide as its text, up to the
		// whole line, so the badge sits right after it
		if (badge.empty())
		{
			m_title.setWidth(LV_PCT(100));
			m_title.removeLocalStyleProp(LV_STYLE_MAX_WIDTH);
		}
		else
		{
			m_title.setWidth(LV_SIZE_CONTENT);
			m_title.setMaxWidth(LV_PCT(100));
		}
	}

	void DsSetting::setHint(std::string_view hint)
	{
		m_hint.setText(hint);
		m_hint.setFlag(LV_OBJ_FLAG_HIDDEN, hint.empty());
	}
} // namespace UI::Xml

/*
 * DsSettingGroup.cpp
 */

#include "DsSettingGroup.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Runtime/Translations.h"
#include "src/xml/lv_xml_base_types.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include "utils/utils.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsSettingGroup::DsSettingGroup(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		addStyle(Themes::getComponentStyles().setting_section);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_title.addStyle(Themes::getComponentStyles().setting_group_title);
		m_title.setWidth(LV_PCT(100));
		setTitle("");

		m_box.addStyle(Themes::getComponentStyles().setting_group);
		m_box.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		lv_obj_add_event_cb(getRootPtr(), childCreatedCb, LV_EVENT_CHILD_CREATED, this);
		lv_obj_add_event_cb(getRootPtr(), languageChangedCb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, this);
	}

	void DsSettingGroup::registerXml()
	{
		Widgets::add({.name = "ds_setting_group",
					  .extends = "lv_obj",
					  .description =
						  "A group of settings: its title, in capitals, over a box of the rows written inside it "
						  "(`<ds_setting>`, or a `<ds_list>` of them with the `setting_list` style). Hiding the group "
						  "hides its title too.",
					  .example = R"(<ds_setting_group title="Machine control">
	<ds_setting title="Confirm before acting">
		<ds_toggle/>
	</ds_setting>
	<ds_setting title="Absolute moves use machine coordinates">
		<ds_toggle/>
	</ds_setting>
</ds_setting_group>)",
					  .attributes = {{"title", "The group's title, if there is no `title_tag`"},
									 {"title_tag",
									  "Translation key for the title; follows language changes",
									  AttributeKind::Translation},
									 {"danger",
									  "\"true\" for a group whose actions can't be undone, such as erasing every "
									  "setting: the box is edged in the error colour (`setting_group_danger`)"}}},
					 xmlCreate,
					 xmlApply);
	}

	void* DsSettingGroup::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* group = new DsSettingGroup(attrOr(attrs, "name", "group"), parentOf(state));
		deleteWith(group->getRootPtr(), group);
		return group->getRootPtr();
	}

	void DsSettingGroup::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		auto* group = static_cast<DsSettingGroup*>(LvObj::fromPtr(itemOf(state)));
		if (const char* tag = lv_xml_get_value_of(attrs, "title_tag"))
		{
			group->m_titleTag = tag;
			group->setTitle(Translations::translate(tag));
		}
		else if (const char* title = lv_xml_get_value_of(attrs, "title"))
		{
			group->setTitle(title);
		}
		if (const char* danger = lv_xml_get_value_of(attrs, "danger"); danger != nullptr && lv_xml_to_bool(danger))
		{
			group->m_box.addStyle(Themes::getComponentStyles().setting_group_danger);
		}
	}

	void DsSettingGroup::childCreatedCb(lv_event_t* e)
	{
		// The event comes up from every widget created below the group, and with no child when one is moved
		auto* group = static_cast<DsSettingGroup*>(lv_event_get_user_data(e));
		auto* child = static_cast<lv_obj_t*>(lv_event_get_param(e));
		if (child == nullptr || lv_obj_get_parent(child) != group->getRootPtr() ||
			child == group->m_title.getRootPtr() || child == group->m_box.getRootPtr())
		{
			return;
		}
		lv_obj_set_parent(child, group->m_box.getRootPtr());
	}

	void DsSettingGroup::languageChangedCb(lv_event_t* e)
	{
		auto* group = static_cast<DsSettingGroup*>(lv_event_get_user_data(e));
		if (!group->m_titleTag.empty())
		{
			group->setTitle(Translations::translate(group->m_titleTag));
		}
	}

	void DsSettingGroup::setTitle(std::string_view title)
	{
		m_title.setText(utils::toUpper(title));
		m_title.setFlag(LV_OBJ_FLAG_HIDDEN, title.empty());
	}
} // namespace UI::Xml

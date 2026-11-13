/*
 * test_settings.cpp
 *
 * The settings tabs (assets/ui/screens/settings_*.xml, src/UI/Screens/Settings/).
 */

#include "ObjectModel/Job.h"
#include "Storage.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <gtest/gtest.h>

class TestGeneralSettings : public UiTestSuite
{
  protected:
	TestGeneralSettings()
	{
		tab.setSize(800, 600);
		tab.show();
	}

	/** The widget called `name` in the tab */
	lv_obj_t* part(const char* name) { return lv_obj_find_by_name(tab.getRootPtr(), name); }

	/** The dropdown of the setting called `name` */
	lv_obj_t* dropdownOf(const char* name) { return lv_obj_find_by_name(part(name), "dropdown"); }

	/** Pick option `index` of `dropdown`, as the user would */
	static void pick(lv_obj_t* dropdown, uint32_t index)
	{
		lv_dropdown_set_selected(dropdown, index);
		lv_obj_send_event(dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
	}

	UI::Xml::XmlView tab{"settings_general", screen, "settings_general"};
};

TEST_F(TestGeneralSettings, TogglesAreStored)
{
	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, true);
	tab.hide();
	tab.show(); // Read again
	lv_obj_t* toggle = part("show_confirmation_dialogs");
	ASSERT_NE(toggle, nullptr);
	EXPECT_TRUE(lv_obj_has_state(toggle, LV_STATE_CHECKED));

	lv_obj_remove_state(toggle, LV_STATE_CHECKED);
	lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_FALSE(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS));

	lv_obj_add_state(toggle, LV_STATE_CHECKED);
	lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_TRUE(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS));
}

TEST_F(TestGeneralSettings, DropdownChoicesAreStored)
{
	const auto stored = StorageHelper::getData(ID_JOB_PROGRESS_SOURCE);
	pick(dropdownOf("job_progress_source"), 1);
	EXPECT_EQ(StorageHelper::getData(ID_JOB_PROGRESS_SOURCE), OM::JobProgressSource(1));
	pick(dropdownOf("job_progress_source"), static_cast<uint32_t>(stored));
}

TEST_F(TestGeneralSettings, ALanguageIsOnlyChangedAfterAsking)
{
	lv_obj_t* language = dropdownOf("language");
	ASSERT_NE(language, nullptr);
	const auto current = static_cast<uint32_t>(i18n::getLanguageIndex(i18n::getCurrentLanguage()));
	ASSERT_GE(lv_dropdown_get_option_count(language), 2u);
	EXPECT_EQ(lv_dropdown_get_selected(language), current);

	lv_obj_t* dialog = part("language_confirm");
	ASSERT_NE(dialog, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
	pick(language, current == 0 ? 1 : 0);
	EXPECT_FALSE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN)) << "asks before restarting";

	lv_obj_send_event(lv_obj_find_by_name(dialog, "cancel"), LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
	EXPECT_EQ(lv_dropdown_get_selected(language), current) << "No shows the language in use again";
}

class TestDisplaySettings : public UiTestSuite
{
  protected:
	TestDisplaySettings()
	{
		tab.setSize(800, 600);
		tab.show();
	}

	UI::Xml::XmlView tab{"settings_display", screen, "settings_display"};
};

TEST_F(TestDisplaySettings, PickingAThemeUsesAndStoresIt)
{
	lv_obj_t* themes = lv_obj_find_by_name(lv_obj_find_by_name(tab.getRootPtr(), "theme"), "dropdown");
	ASSERT_NE(themes, nullptr);
	ASSERT_GE(lv_dropdown_get_option_count(themes), 2u);
	const uint32_t original = lv_dropdown_get_selected(themes);
	const uint32_t other = original == 0 ? 1 : 0;

	lv_dropdown_set_selected(themes, other);
	lv_obj_send_event(themes, LV_EVENT_VALUE_CHANGED, nullptr);
	ASSERT_NE(UI::Themes::getTheme(other), nullptr);
	EXPECT_EQ(UI::Themes::getCurrentTheme(), UI::Themes::getTheme(other));
	EXPECT_EQ(StorageHelper::getData(ID_THEME), UI::Themes::getTheme(other)->getName());

	lv_dropdown_set_selected(themes, original);
	lv_obj_send_event(themes, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_EQ(UI::Themes::getCurrentTheme(), UI::Themes::getTheme(original));
}

class TestDeveloperSettings : public UiTestSuite
{
  protected:
	TestDeveloperSettings()
	{
		tab.setSize(800, 600);
		tab.show();
	}

	UI::Xml::XmlView tab{"settings_developer", screen, "settings_developer"};
};

TEST_F(TestDeveloperSettings, TogglesAreStoredAndBuildOnlyOptionsHidden)
{
	const bool original = StorageHelper::getData(ID_ENABLE_ADVANCED_SETTINGS);
	lv_obj_t* toggle = lv_obj_find_by_name(tab.getRootPtr(), "enable_advanced_settings");
	ASSERT_NE(toggle, nullptr);
	lv_obj_set_state(toggle, LV_STATE_CHECKED, !original);
	lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_EQ(StorageHelper::getData(ID_ENABLE_ADVANCED_SETTINGS), !original);
	lv_obj_set_state(toggle, LV_STATE_CHECKED, original);
	lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);

	EXPECT_EQ(lv_obj_has_flag(lv_obj_find_by_name(tab.getRootPtr(), "enable_ssh"), LV_OBJ_FLAG_HIDDEN),
			  !DEVELOPER_MODE);
	EXPECT_EQ(lv_obj_has_flag(lv_obj_find_by_name(tab.getRootPtr(), "debug_borders"), LV_OBJ_FLAG_HIDDEN),
			  !DEBUG_BORDERS);
}

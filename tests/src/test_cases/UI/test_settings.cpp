/*
 * test_settings.cpp
 *
 * The settings pages (assets/ui/components/settings/settings_*.xml, src/UI/Presenters/Settings/).
 */

#include "ObjectModel/JobProgressSource.h"
#include "Storage.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/View/Actions.h"
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"
#include "test_utils/Dialogs.h"
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

	/** The choice of the setting called `name` */
	lv_obj_t* choiceOf(const char* name) { return lv_obj_find_by_name(part(name), "choice"); }

	UI::Xml::XmlView tab{"settings_general", screen, "settings_general"};
};

TEST_F(TestGeneralSettings, TogglesAreStored)
{
	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, true);
	tab.hide();
	tab.show(); // Read again
	lv_obj_t* toggle = lv_obj_find_by_name(part("show_confirmation_dialogs"), "switch");
	ASSERT_NE(toggle, nullptr);
	EXPECT_TRUE(lv_obj_has_state(toggle, LV_STATE_CHECKED));

	lv_obj_remove_state(toggle, LV_STATE_CHECKED);
	lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_FALSE(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS));

	lv_obj_add_state(toggle, LV_STATE_CHECKED);
	lv_obj_send_event(toggle, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_TRUE(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS));
}

TEST_F(TestGeneralSettings, ChoicesAreStored)
{
	const auto stored = static_cast<uint32_t>(StorageHelper::getData(ID_KEYBOARD_LAYOUT) == "gb" ? 1 : 0);
	lv_obj_send_event(choiceOf("keyboard_layout"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(TestDialogs::chosenIndex(), static_cast<int32_t>(stored));
	TestDialogs::choose(1);
	EXPECT_EQ(StorageHelper::getData(ID_KEYBOARD_LAYOUT), "gb");

	lv_obj_send_event(choiceOf("keyboard_layout"), LV_EVENT_CLICKED, nullptr);
	TestDialogs::choose(stored);
	runTimers(); // The prompt gone is deleted after the event that answered it
}

TEST_F(TestGeneralSettings, OneOfAFewIsChosenWithItsButton)
{
	const auto stored = StorageHelper::getData(ID_JOB_PROGRESS_SOURCE);
	lv_obj_t* file = lv_obj_find_by_name(part("job_progress_source"), "file");
	lv_obj_t* duration = lv_obj_find_by_name(part("job_progress_source"), "duration");
	lv_obj_send_event(file, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(StorageHelper::getData(ID_JOB_PROGRESS_SOURCE), OM::JobProgressSource(1));
	EXPECT_TRUE(lv_obj_has_state(file, LV_STATE_CHECKED));
	EXPECT_FALSE(lv_obj_has_state(duration, LV_STATE_CHECKED));
	lv_obj_send_event(duration, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(StorageHelper::getData(ID_JOB_PROGRESS_SOURCE), OM::JobProgressSource(0));
	EXPECT_TRUE(lv_obj_has_state(duration, LV_STATE_CHECKED));
	StorageHelper::setData(ID_JOB_PROGRESS_SOURCE, stored);
}

TEST_F(TestGeneralSettings, ALanguageIsOnlyChangedAfterAsking)
{
	lv_obj_t* language = choiceOf("language");
	ASSERT_NE(language, nullptr);
	const auto current = static_cast<uint32_t>(i18n::getLanguageIndex(i18n::getCurrentLanguage()));

	EXPECT_EQ(TestDialogs::shown(), nullptr);
	lv_obj_send_event(language, LV_EVENT_CLICKED, nullptr);
	ASSERT_GE(TestDialogs::choiceCount(), 2u);
	EXPECT_EQ(TestDialogs::chosenIndex(), static_cast<int32_t>(current));
	TestDialogs::choose(current == 0 ? 1 : 0);
	ASSERT_NE(TestDialogs::shownNamed("dialog_question"), nullptr) << "asks before restarting";
	EXPECT_EQ(TestDialogs::title(), _("settings.language_confirm.title"));

	TestDialogs::tap("no");
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	runTimers();
	lv_obj_send_event(language, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(TestDialogs::chosenIndex(), static_cast<int32_t>(current)) << "No keeps the language in use";
	TestDialogs::tapHeader("close");
	runTimers();
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
	lv_obj_t* themes = lv_obj_find_by_name(lv_obj_find_by_name(tab.getRootPtr(), "theme"), "choice");
	ASSERT_NE(themes, nullptr);
	lv_obj_send_event(themes, LV_EVENT_CLICKED, nullptr);
	ASSERT_GE(TestDialogs::choiceCount(), 2u);
	const auto original = static_cast<uint32_t>(TestDialogs::chosenIndex());
	const uint32_t other = original == 0 ? 1 : 0;

	TestDialogs::choose(other);
	ASSERT_NE(UI::Themes::getTheme(other), nullptr);
	EXPECT_EQ(UI::Themes::getCurrentTheme(), UI::Themes::getTheme(other));
	EXPECT_EQ(StorageHelper::getData(ID_THEME), UI::Themes::getTheme(other)->getName());
	runTimers();

	lv_obj_send_event(themes, LV_EVENT_CLICKED, nullptr);
	TestDialogs::choose(original);
	EXPECT_EQ(UI::Themes::getCurrentTheme(), UI::Themes::getTheme(original));
	runTimers();
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
	lv_obj_t* toggle = lv_obj_find_by_name(lv_obj_find_by_name(tab.getRootPtr(), "enable_advanced_settings"), "switch");
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

TEST_F(TestDisplaySettings, DarkModeShowsForAThemeWhosePaletteCanChange)
{
	lv_obj_t* row = lv_obj_find_by_name(tab.getRootPtr(), "dark_mode");
	ASSERT_NE(row, nullptr);
	UI::Themes::Theme* theme = UI::Themes::getCurrentTheme();
	ASSERT_NE(theme, nullptr);
	ASSERT_TRUE(theme->isCustomisable()) << "the default theme's palette can change";
	EXPECT_FALSE(lv_obj_has_flag(row, LV_OBJ_FLAG_HIDDEN));

	// Tapping the row turns dark mode over, in the theme's palette
	const bool dark = theme->getPalette().dark;
	lv_obj_send_event(row, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(theme->getPalette().dark, !dark);
	lv_obj_send_event(row, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(theme->getPalette().dark, dark);
	theme->resetPalette();
	UI::Themes::refreshCurrentTheme();
}

class TestNotificationSettings : public UiTestSuite
{
  protected:
	TestNotificationSettings()
	{
		tab.setSize(800, 600);
		tab.show();
	}

	UI::Xml::XmlView tab{"settings_notifications", screen, "settings_notifications"};
};

TEST_F(TestNotificationSettings, TheTimeoutIsSetInSecondsAndStoredInMs)
{
	const auto stored = StorageHelper::getData(ID_NOTIFICATION_TIMEOUT);
	lv_obj_t* slider = lv_obj_find_by_name(lv_obj_find_by_name(tab.getRootPtr(), "notification_timeout"), "slider");
	ASSERT_NE(slider, nullptr);
	float seconds = 2.5f;
	lv_obj_send_event(slider, UI::Xml::Actions::valueConfirmedEvent(), &seconds);
	EXPECT_EQ(StorageHelper::getData(ID_NOTIFICATION_TIMEOUT), std::chrono::milliseconds(2500));
	StorageHelper::setData(ID_NOTIFICATION_TIMEOUT, stored);
}

class TestConnectionSettings : public UiTestSuite
{
  protected:
	/** Show the page, with advanced settings shown or not */
	void show(bool advanced)
	{
		StorageHelper::setData(ID_ENABLE_ADVANCED_SETTINGS, advanced);
		tab.setSize(800, 600);
		tab.show();
	}

	/** Whether the row called `name` is hidden */
	bool hidden(const char* name)
	{
		return lv_obj_has_flag(lv_obj_find_by_name(tab.getRootPtr(), name), LV_OBJ_FLAG_HIDDEN);
	}

	UI::Xml::XmlView tab{"settings_connection", screen, "settings_connection"};
};

TEST_F(TestConnectionSettings, TheUsbPortAndPollIntervalShowOnlyWithAdvancedSettings)
{
	const bool original = StorageHelper::getData(ID_ENABLE_ADVANCED_SETTINGS);
	show(false);
	EXPECT_TRUE(hidden("usb_mode"));
	EXPECT_TRUE(hidden("poll_interval"));
	tab.hide();
	show(true);
	EXPECT_FALSE(hidden("usb_mode"));
	EXPECT_FALSE(hidden("poll_interval"));
	StorageHelper::setData(ID_ENABLE_ADVANCED_SETTINGS, original);
}

class TestSystemSettings : public UiTestSuite
{
  protected:
	TestSystemSettings()
	{
		tab.setSize(800, 600);
		tab.show();
	}

	UI::Xml::XmlView tab{"settings_system", screen, "settings_system"};
};

TEST_F(TestSystemSettings, ErasingAsksFirst)
{
	lv_obj_send_event(
		lv_obj_find_by_name(lv_obj_find_by_name(tab.getRootPtr(), "erase"), "button"), LV_EVENT_CLICKED, nullptr);
	ASSERT_NE(TestDialogs::shownNamed("dialog_question"), nullptr);
	EXPECT_EQ(TestDialogs::title(), _("settings.erase_confirm.title"));
	TestDialogs::tap("no"); // Erasing would restart the program
	EXPECT_EQ(TestDialogs::shown(), nullptr);
	runTimers();
}

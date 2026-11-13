/*
 * test_home_view.cpp
 *
 *  Created on: 2025-08-14
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/BedOrChamber.h"
#include "ObjectModel/Fan.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "Storage.h"
#include "UI/Components/Containers/TabView.h"
#include "UI/Components/Input/Slider.h"
#include "UI/Components/LVGL/LvTextarea.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Notifications/NotificationsPresenter.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/TemperatureSource.h"
#include "UI/Xml/Runtime/Services.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "UI/Xml/Widgets/DsGraph.h"
#include "helix_xml.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include "utils/UpgradeHelper.h"
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>

class TestHomeView : public UiTestSuite
{
  protected:
	/** The home screen, as the application shows it */
	TestHomeView()
		: home("home", lv_screen_active(), "home")
	{
		ZoneScoped;
		home.show();
	}

	/** The view home.xml registers as `name`; fails the test if there is none */
	template <typename T>
	static T& page(std::string_view name)
	{
		T* view = UI::findView<T>(name);
		if (view == nullptr)
		{
			throw std::runtime_error(fmt::format("home has no {:s}", name));
		}
		return *view;
	}

	/** The dashboard's tabs (jobs, macros, status), as dashboard.xml builds them */
	static UI::TabView& dashboardTabs()
	{
		lv_obj_t* tabs = lv_obj_find_by_name(page<UI::LvObj>("dashboard").getRootPtr(), "dashboard_tabs");
		if (tabs == nullptr)
		{
			throw std::runtime_error("the dashboard has no tabs");
		}
		return static_cast<UI::TabView&>(*UI::LvObj::fromPtr(tabs));
	}

	static constexpr size_t STATUS_TAB = 2; // Index of the dashboard's Status tab

	/** The dashboard's temperature graph, as dashboard.xml builds it */
	static UI::Xml::DsGraph& temperatureGraph()
	{
		lv_obj_t* graph = lv_obj_find_by_name(page<UI::LvObj>("dashboard").getRootPtr(), "temperature_graph");
		if (graph == nullptr)
		{
			throw std::runtime_error("the dashboard has no temperature_graph");
		}
		return static_cast<UI::Xml::DsGraph&>(*UI::LvObj::fromPtr(graph));
	}

	/** The Control page's tabs, as control.xml builds them */
	static UI::TabView& controlPage()
	{
		lv_obj_t* tabs = lv_obj_find_by_name(page<UI::LvObj>("control").getRootPtr(), "tabs");
		if (tabs == nullptr)
		{
			throw std::runtime_error("the Control page has no tabs");
		}
		return static_cast<UI::TabView&>(*UI::LvObj::fromPtr(tabs));
	}

	/** Open the Control page on tab `tab` (0 move, 1 temperature, 2 height map, 3 object cancel, 4 fans) */
	static void openControlTab(size_t tab)
	{
		UI::openScreen(std::string_view("control"), false);
		controlPage().setActiveTab(tab);
	}
	/** The widget called `name` on the console page */
	static lv_obj_t* consolePart(const char* name)
	{
		return lv_obj_find_by_name(page<UI::LvObj>("console").getRootPtr(), name);
	}

	/** The Files page's tabs, as files.xml builds them */
	static UI::TabView& filesPage()
	{
		lv_obj_t* tabs = lv_obj_find_by_name(page<UI::LvObj>("files").getRootPtr(), "tabs");
		if (tabs == nullptr)
		{
			throw std::runtime_error("the Files page has no tabs");
		}
		return static_cast<UI::TabView&>(*UI::LvObj::fromPtr(tabs));
	}

	/** Open the Files page on tab `tab` (0 macros, 1 jobs) */
	static void openFilesTab(size_t tab)
	{
		UI::openScreen(std::string_view("files"), false);
		filesPage().setActiveTab(tab);
	}

	/**
	 * Show `listing` (an rr_filelist reply) in the file browser registered as `browser`, as if the printer had
	 * replied to its refresh button
	 */
	static lv_obj_t* showListing(const char* browser, const char* listing)
	{
		load_model_data_from_file("tests/object_model/test_bench/model_directories_v.json");
		lv_obj_t* root = page<UI::LvObj>(browser).getRootPtr();
		lv_obj_send_event(lv_obj_find_by_name(root, "refresh"), LV_EVENT_CLICKED, nullptr);
		load_model_data_from_file(listing);
		return root;
	}

	/** The item showing the file called `name` in a file browser, in whichever list (rows or tiles) is shown */
	static lv_obj_t* fileItem(lv_obj_t* browser, std::string_view name)
	{
		for (const char* listName : {"rows", "tiles"})
		{
			lv_obj_t* list = lv_obj_find_by_name(browser, listName);
			if (list == nullptr || lv_obj_has_flag(list, LV_OBJ_FLAG_HIDDEN))
			{
				continue;
			}
			for (uint32_t i = 0; i < lv_obj_get_child_count(list); i++)
			{
				lv_obj_t* item = lv_obj_get_child(list, static_cast<int32_t>(i));
				lv_obj_t* label = lv_obj_find_by_name(item, "label");
				if (label != nullptr && lv_label_get_text(label) == name)
				{
					return item;
				}
			}
		}
		throw std::runtime_error(fmt::format("no file item shows {:s}", name));
	}

	/** The open file dialog (each file browser has one, in the modal host), or nullptr */
	static lv_obj_t* openFileDialog()
	{
		lv_obj_t* host = UI::getModalHost()->getRootPtr();
		for (uint32_t i = 0; i < lv_obj_get_child_count(host); i++)
		{
			lv_obj_t* backdrop = lv_obj_get_child(host, static_cast<int32_t>(i));
			lv_obj_t* dialog = lv_obj_find_by_name(backdrop, "file_browser_dialog");
			if (dialog != nullptr && !lv_obj_has_flag(backdrop, LV_OBJ_FLAG_HIDDEN) &&
				!lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN))
			{
				return dialog;
			}
		}
		return nullptr;
	}

	static std::string_view labelText(lv_obj_t* parent, const char* name)
	{
		return lv_label_get_text(lv_obj_find_by_name(parent, name));
	}

	static bool isHidden(lv_obj_t* parent, const char* name)
	{
		return lv_obj_has_flag(lv_obj_find_by_name(parent, name), LV_OBJ_FLAG_HIDDEN);
	}
	/** The Settings page's tabs (0 general, 1 connection, 2 display, 3 developer), as settings.xml builds them
	 */
	static UI::TabView& settingsPage()
	{
		lv_obj_t* tabs = lv_obj_find_by_name(page<UI::LvObj>("settings").getRootPtr(), "tabs");
		if (tabs == nullptr)
		{
			throw std::runtime_error("the Settings page has no tabs");
		}
		return static_cast<UI::TabView&>(*UI::LvObj::fromPtr(tabs));
	}
	/** What notifications.xml shows responses, alerts and update prompts with */
	static UI::NotificationsPresenter& notifications()
	{
		return static_cast<UI::NotificationsPresenter&>(*page<UI::Xml::XmlView>("notifications").getPresenter());
	}

	/** The widget called `name` in the notifications */
	static lv_obj_t* notificationPart(const char* name)
	{
		return lv_obj_find_by_name(page<UI::LvObj>("notifications").getRootPtr(), name);
	}

	/** The update prompt's checks shared by the update result tests */
	static void expectUpdateResult()
	{
		lv_obj_t* prompt = notificationPart("update_prompt");
		ASSERT_NE(prompt, nullptr);
		EXPECT_FALSE(lv_obj_has_flag(prompt, LV_OBJ_FLAG_HIDDEN));
		EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(prompt, "ok"), LV_OBJ_FLAG_HIDDEN));
		EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(prompt, "cancel"), LV_OBJ_FLAG_HIDDEN));
		EXPECT_STRNE(lv_label_get_text(lv_obj_find_by_name(prompt, "title")), "");
		EXPECT_STRNE(lv_label_get_text(lv_obj_find_by_name(prompt, "text")), "");
	}

	/** The M291 alert's text box */
	static UI::LvTextarea& alertInput()
	{
		lv_obj_t* input = notificationPart("input");
		if (input == nullptr)
		{
			throw std::runtime_error("the alert has no input");
		}
		return static_cast<UI::LvTextarea&>(*UI::LvObj::fromPtr(input));
	}

	UI::Xml::XmlView home;
};

TEST_F(TestHomeView, BlankView)
{
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/blank.png")

	lv_obj_t* keyboard = lv_obj_find_by_name(home.getRootPtr(), "keyboard");
	ASSERT_NE(keyboard, nullptr);
	lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
	lv_obj_move_foreground(keyboard);
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/keyboard.png");
}

TEST_F(TestHomeView, BlankControlView)
{
	auto& control = controlPage();

	openControlTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_blank.png");

	control.setActiveTab(1);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/temperature_blank.png");

	control.setActiveTab(2);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/heightmap_blank.png");

	control.setActiveTab(3);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel_blank.png");

	control.setActiveTab(4);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/fan_blank.png");

	control.setActiveTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_blank.png");
}

TEST_F(TestHomeView, ObjectCancelCanvasTitleTracksCurrentObject)
{
	openControlTab(3);

	OM::ClearJobObjects();
	auto object = OM::GetOrCreateJobObject(0);
	ASSERT_NE(object, nullptr);
	object->index = 0;
	object->name = "gear (Instance 5)";
	object->bounds.x[0] = 100;
	object->bounds.x[1] = 150;
	object->bounds.y[0] = 100;
	object->bounds.y[1] = 150;
	OM::SetCurrentJobObject(0);
	Model::get().post<EventType::JobCurrentObject>();
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel/set_object_name.png");

	OM::SetCurrentJobObject(-1);
	Model::get().post<EventType::JobCurrentObject>();
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel/clear_object_name.png");

	OM::ClearJobObjects();
}

TEST_F(TestHomeView, ObjectCancelCurrentButtonRespectsConfirmationSetting)
{
	OM::ClearJobObjects();
	auto object = OM::GetOrCreateJobObject(0);
	ASSERT_NE(object, nullptr);
	object->index = 0;
	object->name = "gear (Instance 1)";
	OM::SetCurrentJobObject(0);

	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, true);
	openControlTab(3);
	lv_obj_t* page = UI::findView("object_cancel")->getRootPtr();
	lv_obj_t* dialog = lv_obj_find_by_name(page, "confirm_object_cancel");
	ASSERT_NE(dialog, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));

	lv_obj_send_event(lv_obj_find_by_name(page, "cancel_current"), LV_EVENT_CLICKED, nullptr);
	EXPECT_FALSE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
	EXPECT_EQ(std::string(lv_label_get_text(lv_obj_find_by_name(dialog, "title"))),
			  _("object_cancel.confirm_cancel_current_title"));
	EXPECT_NE(std::string(lv_label_get_text(lv_obj_find_by_name(dialog, "text"))).find("gear (Instance 1)"),
			  std::string::npos);
	lv_obj_send_event(lv_obj_find_by_name(dialog, "cancel"), LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));

	// Pressing the object on the bed asks to cancel it
	lv_obj_t* rects = lv_obj_find_by_name(page, "objects");
	ASSERT_NE(rects, nullptr);
	ASSERT_EQ(lv_obj_get_child_count(rects), 1u);
	lv_obj_send_event(lv_obj_get_child(rects, 0), LV_EVENT_PRESSED, nullptr);
	EXPECT_FALSE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
	EXPECT_EQ(std::string(lv_label_get_text(lv_obj_find_by_name(dialog, "title"))),
			  _("object_cancel.confirm_cancel_title"));
	lv_obj_send_event(lv_obj_find_by_name(dialog, "cancel"), LV_EVENT_CLICKED, nullptr);

	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, false);
	lv_obj_send_event(lv_obj_find_by_name(page, "cancel_current"), LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN)) << "cancelled without asking";

	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, true);
	OM::SetCurrentJobObject(-1);
	OM::ClearJobObjects();
}

TEST_F(TestHomeView, BlankConsoleView)
{
	UI::openScreen(std::string_view("console"), false);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/blank.png")
}

TEST_F(TestHomeView, BlankMacroView)
{
	openFilesTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/macros_blank.png");
}

TEST_F(TestHomeView, BlankJobView)
{
	openFilesTab(1);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/jobs_blank.png");
}

TEST_F(TestHomeView, JobModalShowsDeleteButton)
{
	openFilesTab(1);
	lv_obj_t* browser = showListing("files_jobs", "tests/object_model/rr_filelist/gcodes.json");
	lv_obj_send_event(fileItem(browser, "test.gcode"), LV_EVENT_CLICKED, nullptr);

	lv_obj_t* dialog = openFileDialog();
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(labelText(dialog, "title"), "Start Print");
	EXPECT_EQ(labelText(dialog, "text"), "File: test.gcode");
	EXPECT_FALSE(isHidden(dialog, "delete"));
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/start_print_modal.png");

	lv_obj_send_event(lv_obj_find_by_name(dialog, "cancel"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(openFileDialog(), nullptr);
}

TEST_F(TestHomeView, MacroModalHidesDeleteButton)
{
	openFilesTab(0);
	lv_obj_t* browser = showListing("files_macros", "tests/object_model/rr_filelist/macros.json");
	lv_obj_send_event(fileItem(browser, "test.g"), LV_EVENT_CLICKED, nullptr); // "01_test.g", ordering prefix hidden

	lv_obj_t* dialog = openFileDialog();
	ASSERT_NE(dialog, nullptr);
	EXPECT_EQ(labelText(dialog, "title"), "Run Macro");
	EXPECT_TRUE(isHidden(dialog, "delete"));
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/run_macro_modal.png");

	UI::closeAllModals();
	EXPECT_EQ(openFileDialog(), nullptr);
}

TEST_F(TestHomeView, DeleteButtonOpensDeleteConfirmationModal)
{
	openFilesTab(1);
	lv_obj_t* browser = showListing("files_jobs", "tests/object_model/rr_filelist/gcodes.json");
	lv_obj_send_event(fileItem(browser, "test.gcode"), LV_EVENT_CLICKED, nullptr);

	lv_obj_t* dialog = openFileDialog();
	ASSERT_NE(dialog, nullptr);
	lv_obj_send_event(lv_obj_find_by_name(dialog, "delete"), LV_EVENT_CLICKED, nullptr);

	ASSERT_EQ(openFileDialog(), dialog) << "the same dialog asks again";
	EXPECT_EQ(labelText(dialog, "title"), "Delete File");
	EXPECT_EQ(labelText(dialog, "text"), "Are you sure you want to delete test.gcode?");
	EXPECT_TRUE(isHidden(dialog, "delete"));
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/confirm_delete_modal.png");

	UI::closeAllModals();
}

TEST_F(TestHomeView, FoldersOpenAndTheirPathLeadsBack)
{
	openFilesTab(1);
	lv_obj_t* browser = showListing("files_jobs", "tests/object_model/rr_filelist/gcodes.json");
	lv_obj_t* crumbs = lv_obj_find_by_name(browser, "breadcrumb_container");
	ASSERT_EQ(lv_obj_get_child_count(crumbs), 1u);

	lv_obj_send_event(fileItem(browser, "Folder"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(openFileDialog(), nullptr) << "a folder opens, it doesn't ask";
	ASSERT_EQ(lv_obj_get_child_count(crumbs), 2u);
	lv_obj_t* base = lv_obj_get_child(crumbs, 0);
	lv_obj_t* folder = lv_obj_get_child(crumbs, 1);
	EXPECT_FALSE(isHidden(base, "folder")) << "the base folder can be tapped";
	EXPECT_EQ(labelText(lv_obj_find_by_name(base, "folder"), "label"), "0:/gcodes");
	EXPECT_EQ(labelText(folder, "open_folder"), "Folder");
	EXPECT_FALSE(isHidden(folder, "separator"));

	lv_obj_send_event(lv_obj_find_by_name(base, "folder"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(lv_obj_get_child_count(crumbs), 1u);
}

TEST_F(TestHomeView, BlankStatusView)
{
	dashboardTabs().setActiveTab(STATUS_TAB);
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/blank.png")
}

TEST_F(TestHomeView, SidebarOpensPagesAndShowsWhichIsOpen)
{
	lv_obj_t* sidebar = UI::findView("sidebar")->getRootPtr();
	auto button = [sidebar](const char* name) { return lv_obj_find_by_name(sidebar, name); };
	auto pump = []
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	};
	pump();
	EXPECT_TRUE(lv_obj_has_state(button("home"), LV_STATE_CHECKED));

	// Each button raises nav.open with its page's name
	lv_obj_send_event(button("control"), LV_EVENT_CLICKED, nullptr);
	pump();
	EXPECT_TRUE(UI::findView("control")->isVisible());
	EXPECT_FALSE(UI::findView("dashboard")->isVisible());
	EXPECT_TRUE(lv_obj_has_state(button("control"), LV_STATE_CHECKED));
	EXPECT_FALSE(lv_obj_has_state(button("home"), LV_STATE_CHECKED));

	lv_obj_send_event(button("settings"), LV_EVENT_CLICKED, nullptr);
	pump();
	EXPECT_TRUE(UI::findView("settings")->isVisible());
	EXPECT_FALSE(UI::findView("control")->isVisible());
	EXPECT_TRUE(lv_obj_has_state(button("settings"), LV_STATE_CHECKED));
	EXPECT_FALSE(lv_obj_has_state(button("control"), LV_STATE_CHECKED));

	// nav.home closes them all
	lv_obj_send_event(button("home"), LV_EVENT_CLICKED, nullptr);
	pump();
	EXPECT_TRUE(UI::findView("dashboard")->isVisible());
	EXPECT_FALSE(UI::findView("settings")->isVisible());
	EXPECT_TRUE(lv_obj_has_state(button("home"), LV_STATE_CHECKED));
}

TEST_F(TestHomeView, Response)
{
	notifications().newResponse(ResponseType::INFO, "This is a response message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response.png");
}

TEST_F(TestHomeView, SuccessResponse)
{
	notifications().newResponse(ResponseType::SUCCESS, "This is a success message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_success.png");
}

TEST_F(TestHomeView, WarningResponse)
{
	notifications().newResponse(ResponseType::WARNING, "This is a warning message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_warning.png");
}

TEST_F(TestHomeView, ErrorResponse)
{
	notifications().newResponse(ResponseType::ERROR, "This is an error message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_error.png");
}

TEST_F(TestHomeView, HandleUpdateResultSuccess)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::Success,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v1.0.0",
									.updateBuildrootVersion = "v1.0.0"};

	notifications().handleUpdateResult(info);

	expectUpdateResult();

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/update_success.png");
	UI::closeAllModals();
}

TEST_F(TestHomeView, HandleUpdateResultBuildrootError)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::BuildrootVersionError,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v0.1.0",
									.updateBuildrootVersion = "v1.0.0"};

	notifications().handleUpdateResult(info);

	expectUpdateResult();

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/update_failed.png");
	UI::closeAllModals();
}

TEST_F(TestHomeView, HandleUpdateResultBuildrootWarning)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::BuildrootVersionWarning,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v1.0.0",
									.updateBuildrootVersion = "v1.0.1"};

	notifications().handleUpdateResult(info);

	expectUpdateResult();

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/update_warning.png");
	UI::closeAllModals();
}

TEST_F(TestHomeView, NewStatusDisablesAndEnablesJobsTab)
{
	auto& files = filesPage();
	ASSERT_NE(files.getTabButton(1), nullptr);

	Model::get().post<EventType::Status>(OM::PrinterStatus::paused);
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	EXPECT_TRUE(files.getTabButton(1)->hasState(LV_STATE_DISABLED)) << "a paused job can't be replaced either";

	Model::get().post<EventType::Status>(OM::PrinterStatus::idle);
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	EXPECT_FALSE(files.getTabButton(1)->hasState(LV_STATE_DISABLED));
}

TEST_F(TestHomeView, AlertS0)
{
	load_model_data_from_file("tests/object_model/m291/s0.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s0.png");
}

TEST_F(TestHomeView, AlertS1)
{
	load_model_data_from_file("tests/object_model/m291/s1.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s1.png");
}

TEST_F(TestHomeView, AlertS2)
{
	load_model_data_from_file("tests/object_model/m291/s2.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s2.png");
}

TEST_F(TestHomeView, AlertS3)
{
	load_model_data_from_file("tests/object_model/m291/s3.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s3.png");

	load_model_data_from_file("tests/object_model/test_bench/model_move_vn.json");
	load_model_data_from_file("tests/object_model/m291/s3_axis.json");
	home.show();
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s3_axis.png");
}

TEST_F(TestHomeView, AlertS4)
{
	load_model_data_from_file("tests/object_model/m291/s4.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s4.png");

	load_model_data_from_file("tests/object_model/m291/s4_j1.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s4_j1.png");
}

TEST_F(TestHomeView, AlertS5)
{
	load_model_data_from_file("tests/object_model/m291/s5.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5.png");

	load_model_data_from_file("tests/object_model/m291/s5_j1.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_j1.png");

	load_model_data_from_file("tests/object_model/m291/s5_limits.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits.png");

	auto& input = alertInput();
	input.sendEvent(LV_EVENT_FOCUSED);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits_input.png");

	input.setText("-1");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits_min.png");

	input.setText("11");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits_max.png");

	input.setText("10");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits_ok.png");

	input.setText("5.5");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits_float.png");
}

TEST_F(TestHomeView, AlertS6)
{
	load_model_data_from_file("tests/object_model/m291/s6.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6.png");

	load_model_data_from_file("tests/object_model/m291/s6_j1.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_j1.png");

	load_model_data_from_file("tests/object_model/m291/s6_limits.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits.png");

	auto& input = alertInput();
	input.sendEvent(LV_EVENT_FOCUSED);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits_input.png");

	input.setText("-0.1");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits_min.png");

	input.setText("10.1");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits_max.png");

	input.setText("10");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits_ok.png");

	input.setText("5.5");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits_float.png");
}

TEST_F(TestHomeView, AlertS7)
{
	load_model_data_from_file("tests/object_model/m291/s7_j1.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_j1.png");

	load_model_data_from_file("tests/object_model/m291/s7_limits.json");
	notifications().newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits.png");

	auto& input = alertInput();
	input.sendEvent(LV_EVENT_FOCUSED);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits_input.png");

	input.setText("short");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits_min.png");

	input.setText("too long..........");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits_max.png");

	input.setText("this is ok");
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits_ok.png");
}

class TestHomeViewWithData : public TestHomeView
{
  protected:
	TestHomeViewWithData()
	{
		ZoneScoped;
		load_model_data_from_file("tests/object_model/test_bench/model_boards_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_directories_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_fans_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_heat_v.json");
		load_model_data_from_file("tests/object_model/job/model_job_printing_layer_2.json");
		load_model_data_from_file("tests/object_model/test_bench/model_move_vn.json");
		load_model_data_from_file("tests/object_model/test_bench/model_network_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_sensors_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_spindles_v.json");
		load_model_data_from_file("tests/object_model/job/model_state_printing.json");
		load_model_data_from_file("tests/object_model/test_bench/model_tools_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_volumes_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_all_d99f.json");
		load_model_data_from_file("tests/object_model/rr_filelist/example1.json");

		home.show();
	}
};

TEST_F(TestHomeViewWithData, Dashboard)
{
	ZoneScoped;
	EXPECT_EQ(OM::Heat::GetHeaterCount(), 4);
	EXPECT_EQ(OM::GetToolCount(), 4);

	/* Populate graph with fake sensor data, as the model ticks would */
	auto* temperatures = UI::Xml::DataSources::find<UI::Xml::TemperatureSource>();
	ASSERT_NE(temperatures, nullptr);
	auto sensor = OM::GetAnalogSensorBySlot(0);
	sensor->lastReading = 25.0f;
	for (size_t i = 0; i < 1000; i++)
	{
		temperatures->sample();
		sensor->lastReading = (int32_t)(sensor->lastReading + 1) % 300;
	}

	temperatureGraph().showSeries(1, false);

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/temperature_graph.png");

	/* Open the tool list numberpad */
	// The first tool's first heater row, as dashboard_tools.xml builds it
	lv_obj_t* toolList = UI::findView("dashboard_tools")->getRootPtr();
	lv_obj_t* firstHeater = lv_obj_find_by_name(lv_obj_find_by_name(toolList, "tools"), "heaters");
	lv_obj_send_event(lv_obj_find_by_name(firstHeater, "active"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/tool_list_numberpad.png");

	UI::closeAllModals();
}

TEST_F(TestHomeViewWithData, ToolListLocksHeatersWhilePrinting)
{
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	lv_obj_t* toolList = UI::findView("dashboard_tools")->getRootPtr();
	lv_obj_t* tools = lv_obj_find_by_name(toolList, "tools");
	lv_obj_t* beds = lv_obj_find_by_name(toolList, "beds");
	ASSERT_NE(lv_obj_find_by_name(tools, "status"), nullptr);
	ASSERT_NE(lv_obj_find_by_name(beds, "status"), nullptr);

	// Printing: nothing that would change the job can be pressed
	EXPECT_TRUE(lv_obj_has_state(lv_obj_find_by_name(tools, "tool_name"), LV_STATE_DISABLED));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_find_by_name(tools, "status"), LV_STATE_DISABLED));
	EXPECT_TRUE(lv_obj_has_state(lv_obj_find_by_name(beds, "status"), LV_STATE_DISABLED));
	// Temperatures can still be set
	EXPECT_FALSE(lv_obj_has_state(lv_obj_find_by_name(tools, "active"), LV_STATE_DISABLED));

	load_model_data_from_file("tests/object_model/job/model_state_paused.json");
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	EXPECT_FALSE(lv_obj_has_state(lv_obj_find_by_name(tools, "status"), LV_STATE_DISABLED));
	EXPECT_FALSE(lv_obj_has_state(lv_obj_find_by_name(beds, "status"), LV_STATE_DISABLED));
}

TEST_F(TestHomeViewWithData, SimplePrinterDashboard)
{
	ZoneScoped;
	load_model_data_from_file("tests/object_model/5_axis/model_heat_v.json");
	load_model_data_from_file("tests/object_model/5_axis/model_tools_v.json");

	EXPECT_EQ(OM::Heat::GetHeaterCount(), 1);
	EXPECT_EQ(OM::GetToolCount(), 1);
	EXPECT_EQ(OM::GetBedCount(), 0);
	EXPECT_EQ(OM::GetChamberCount(), 0);

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/simple_printer.png");
}

TEST_F(TestHomeViewWithData, ConsoleView)
{
	ZoneScoped;
	UI::openScreen(std::string_view("console"));

	// As the Duet and the logger would report them
	Model::get().post<EventType::ConsoleCommand>(std::string("M115"));
	Model::get().post<EventType::Response>(ResponseType::INFO, std::string("Testing response"));
	Model::get().post<EventType::Response>(ResponseType::INFO, std::string("new multi-line response\nline 2"));
	Model::get().post<EventType::Response>(ResponseType::SUCCESS, std::string("Success response"));
	Model::get().post<EventType::Response>(ResponseType::WARNING, std::string("Warning response"));
	Model::get().post<EventType::Response>(ResponseType::ERROR, std::string("Error response"));
	Model::get().post<EventType::LogMessage>(
		Log::DebugLevel::Info, Log::log_time_t{}, std::string("Testing log message"));
	Model::get().startEventLoop();
	Model::get().stopEventLoop();

	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/duet_responses.png")
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/command_list.png");

	lv_obj_send_event(consolePart("command_visibility"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/command_list_hidden.png");

	lv_obj_send_event(consolePart("input"), LV_EVENT_FOCUSED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/keyboard.png");

	// Put back as it was, for the next test
	lv_obj_send_event(consolePart("command_visibility"), LV_EVENT_CLICKED, nullptr);
}

TEST_F(TestHomeViewWithData, ConsoleNarrowsTheGcodesAndSends)
{
	std::vector<std::string> sent;
	UI::Xml::Services::setGcodeSender([&](std::string_view gcode) { sent.emplace_back(gcode); });
	UI::openScreen(std::string_view("console"));
	lv_obj_t* list = consolePart("list");
	auto& input = static_cast<UI::LvTextarea&>(*UI::LvObj::fromPtr(consolePart("input")));

	input.setText("m11");
	lv_obj_t* first = nullptr;
	for (uint32_t i = 0; i < lv_obj_get_child_count(list) && first == nullptr; i++)
	{
		first = lv_obj_find_by_name(lv_obj_get_child(list, static_cast<int32_t>(i)), "gcode");
	}
	ASSERT_NE(first, nullptr);
	EXPECT_TRUE(std::string_view(lv_label_get_text(first)).starts_with("M11")) << "typed lower case";

	// Picking one types it; sending sends what is typed
	lv_obj_send_event(lv_obj_get_parent(first), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(input.getText(), lv_label_get_text(first));
	input.setText("M115 P1");
	lv_obj_send_event(consolePart("enter"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(sent, (std::vector<std::string>{"M115 P1\n"}));
	UI::Xml::Services::setGcodeSender(nullptr);
}

TEST_F(TestHomeViewWithData, ConsoleFollowsNewLines)
{
	UI::openScreen(std::string_view("console"));
	lv_obj_t* output = consolePart("output");
	ASSERT_NE(output, nullptr);

	// More than a screenful, arriving one at a time, some wrapping
	for (int i = 0; i < 40; i++)
	{
		Model::get().post<EventType::Response>(
			ResponseType::INFO,
			fmt::format("response {:d}{:s}", i, i % 3 == 0 ? std::string(150, 'x') : std::string("\nsecond line")));
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
		lv_refr_now(nullptr);

		lv_obj_t* last = lv_obj_get_child(output, -1);
		lv_area_t lastArea;
		lv_area_t outputArea;
		lv_obj_get_coords(last, &lastArea);
		lv_obj_get_content_coords(output, &outputArea);
		EXPECT_GE(lv_obj_get_scroll_top(output), 0) << "line " << i;
		EXPECT_LE(lv_obj_get_scroll_bottom(output), 0) << "line " << i;
		EXPECT_LE(lastArea.y2, outputArea.y2) << "line " << i << ": the newest line is below the view";
		EXPECT_GE(lastArea.y1, outputArea.y1) << "line " << i << ": the newest line is above the view";
	}
}

TEST_F(TestHomeViewWithData, ControlView)
{
	ZoneScoped;
	auto& control = controlPage();

	openControlTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move.png");

	load_model_data_from_file("tests/object_model/test_bench/model_10_axes.json");
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_10_axes.png");
	{
		// Hold the second babystep size to change it
		lv_obj_t* steps = lv_obj_find_by_name(page<UI::LvObj>("move").getRootPtr(), "value_list");
		ASSERT_NE(steps, nullptr);
		ASSERT_GE(lv_obj_get_child_count(steps), 2u);
		lv_obj_send_event(lv_obj_get_child(steps, 1), LV_EVENT_LONG_PRESSED, nullptr);
		EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_babystep_long_press.png");
	}

	control.setActiveTab(1);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/temperature.png");
	{
		lv_obj_t* selector = lv_obj_find_by_name(page<UI::LvObj>("temperature").getRootPtr(), "distance_selector");
		ASSERT_NE(selector, nullptr);
		lv_obj_t* input = lv_obj_find_by_name(selector, "value");
		ASSERT_NE(input, nullptr);
		lv_obj_send_event(input, LV_EVENT_CLICKED, nullptr);
		EXPECT_EQUAL_SCREENSHOT("home_view/control_view/temperature_distance_input.png");
	}

	control.setActiveTab(2);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/heightmap.png");

	control.setActiveTab(4);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/fan.png");

	load_model_data_from_file("tests/object_model/job/model_job_objects.json");
	{
		auto x = OM::Move::GetAxisByLetter('X');
		x->minPosition = -200.0f;
		x->maxPosition = 100.0f;

		auto y = OM::Move::GetAxisByLetter('Y');
		y->minPosition = -100.0f;
		y->maxPosition = 100.0f;
	}
	control.setActiveTab(3);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel.png");

	control.setActiveTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_10_axes.png");
}

TEST_F(TestHomeViewWithData, MacroView)
{
	openFilesTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/macros.png")
}

TEST_F(TestHomeViewWithData, SettingsView)
{
	UI::openScreen(std::string_view("settings"));
	auto& settings = settingsPage();
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/initial.png")

	settings.setActiveTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/general.png")

	settings.setActiveTab(1);
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for wifi networks to load
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/connection.png")

	settings.setActiveTab(2);
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/display.png")

	settings.setActiveTab(3);
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/developer.png")
}

TEST_F(TestHomeViewWithData, WifiAsksForANewNetworksPassword)
{
	UI::openScreen(std::string_view("settings"));
	settingsPage().setActiveTab(1);
	lv_obj_t* wifi = UI::findView("settings_wifi")->getRootPtr();
	lv_obj_t* networks = lv_obj_find_by_name(wifi, "list");
	ASSERT_NE(networks, nullptr);
	for (int i = 0; i < 50 && lv_obj_get_child_count(networks) == 0; i++) // Scanned on a thread of its own
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	}
	ASSERT_GT(lv_obj_get_child_count(networks), 0u);

	// The last network found isn't known, so connecting to it asks for its password
	lv_obj_t* unknown = lv_obj_get_child(networks, -1);
	ASSERT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(unknown, "forget"), LV_OBJ_FLAG_HIDDEN));
	lv_obj_t* dialog = lv_obj_find_by_name(lv_screen_active(), "wifi_password_modal");
	ASSERT_NE(dialog, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
	lv_obj_send_event(lv_obj_find_by_name(unknown, "connect"), LV_EVENT_CLICKED, nullptr);
	EXPECT_FALSE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));

	// Typing the password brings up the keyboard, and the dialog moves out of its way
	lv_obj_t* input = lv_obj_find_by_name(dialog, "wifi_password_input");
	lv_obj_send_event(input, LV_EVENT_FOCUSED, nullptr);
	lv_obj_t* keyboard = lv_obj_find_by_name(home.getRootPtr(), "keyboard");
	ASSERT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));
	lv_obj_update_layout(lv_screen_active());
	lv_area_t dialogArea;
	lv_area_t keyboardArea;
	lv_obj_get_coords(dialog, &dialogArea);
	lv_obj_get_coords(keyboard, &keyboardArea);
	EXPECT_LE(dialogArea.y2, keyboardArea.y1) << "the dialog is above the keyboard";
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/wifi_password_keyboard.png")

	// The eye shows the password, and hides it again
	lv_obj_t* eye = lv_obj_find_by_name(dialog, "show_password");
	ASSERT_NE(eye, nullptr);
	EXPECT_TRUE(lv_textarea_get_password_mode(input));
	lv_obj_add_state(eye, LV_STATE_CHECKED);
	lv_obj_send_event(eye, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_FALSE(lv_textarea_get_password_mode(input));
	lv_obj_remove_state(eye, LV_STATE_CHECKED);
	lv_obj_send_event(eye, LV_EVENT_VALUE_CHANGED, nullptr);
	EXPECT_TRUE(lv_textarea_get_password_mode(input));
	EXPECT_FALSE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN)) << "the keyboard stays up";

	// The keyboard's close key ends the edit: the keyboard goes, and the dialog moves back
	lv_obj_send_event(keyboard, LV_EVENT_CANCEL, nullptr);
	lv_obj_send_event(input, LV_EVENT_CANCEL, nullptr); // As the keyboard sends it on to its box
	EXPECT_TRUE(lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));
	lv_obj_update_layout(lv_screen_active());
	lv_area_t centred;
	lv_obj_get_coords(dialog, &centred);
	EXPECT_GT(centred.y1, dialogArea.y1) << "the dialog is back in the middle";
	lv_obj_send_event(lv_obj_find_by_name(dialog, "cancel"), LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(dialog, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(TestHomeViewWithData, StatusView)
{
	dashboardTabs().setActiveTab(STATUS_TAB);
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/printing.png")

	lv_obj_t* status = page<UI::LvObj>("dashboard_job_status").getRootPtr();
	auto click = [&](const char* name)
	{
		lv_obj_t* obj = lv_obj_find_by_name(status, name);
		ASSERT_NE(obj, nullptr) << name;
		lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
	};

	click("speed_multiplier");
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/speed_factor_numberpad.png")

	UI::closeAllModals();

	click("flow_multiplier");
	OM::Move::SetExtruderFactor(0, 1.2f);
	Model::get().post<EventType::ExtruderData>();
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/extrusion_factor_numberpad.png")

	UI::closeAllModals();

	click("babystep_button");
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/babystep_modal.png")
}

TEST_F(TestHomeViewWithData, ReloadKeepsEveryPageWorking)
{
	// Bundles are reloaded whatever is showing: every page, and an open dialog
	const char* pages[] = {"control", "console", "settings", "files"};
	for (int round = 0; round < 3; round++)
	{
		for (const char* name : pages)
		{
			UI::openScreen(std::string_view(name), false);
			UI::Xml::reload();
			ASSERT_NE(UI::findView(name), nullptr) << name << " is registered again after a reload";
		}

		openFilesTab(1);
		lv_obj_t* browser = showListing("files_jobs", "tests/object_model/rr_filelist/gcodes.json");
		lv_obj_send_event(fileItem(browser, "test.gcode"), LV_EVENT_CLICKED, nullptr);
		ASSERT_NE(openFileDialog(), nullptr);
		UI::Xml::reload();
		EXPECT_EQ(openFileDialog(), nullptr) << "the dialog goes with its browser";

		UI::home();
		dashboardTabs().setActiveTab(STATUS_TAB);
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
		UI::Xml::reload();
		lv_refr_now(nullptr);
	}

	// Rebuilt from scratch, showing the printer as before
	UI::home();
	ASSERT_EQ(dashboardTabs().getTabCount(), 3u);
	lv_obj_t* tools = lv_obj_find_by_name(UI::findView("dashboard_tools")->getRootPtr(), "tools");
	EXPECT_EQ(lv_obj_get_child_count(lv_obj_find_by_name(tools, "list")), 4u) << "the test bench's four tools";
}

TEST_F(TestHomeView, ReloadKeepsTheOpenPageAndTabs)
{
	// On the Control page's Temperature tab, having come from the Display settings, with the dashboard underneath on
	// its Status tab
	dashboardTabs().setActiveTab(STATUS_TAB);
	UI::openScreen(std::string_view("settings"));
	settingsPage().setActiveTab(2);
	UI::openScreen(std::string_view("control"));
	controlPage().setActiveTab(1);

	UI::Xml::reload();

	EXPECT_EQ(UI::getCurrentScreenName(), "control");
	EXPECT_TRUE(page<UI::LvObj>("control").isShownOnScreen());
	EXPECT_FALSE(page<UI::LvObj>("dashboard").isVisible()) << "the home screen stays under the page";
	EXPECT_EQ(controlPage().getActiveTabIndex(), 1u);
	EXPECT_TRUE(page<UI::LvObj>("temperature").isShownOnScreen());
	EXPECT_FALSE(page<UI::LvObj>("move").isShownOnScreen());
	EXPECT_EQ(settingsPage().getActiveTabIndex(), 2u) << "a page that isn't open keeps its tab too";

	UI::back();
	EXPECT_EQ(UI::getCurrentScreenName(), "settings") << "back returns where it did before the reload";
	EXPECT_TRUE(page<UI::LvObj>("settings").isShownOnScreen());

	UI::home();
	EXPECT_EQ(dashboardTabs().getActiveTabIndex(), STATUS_TAB);
}

TEST_F(TestHomeView, ReloadDoesNotMoveFocusThroughTheOldWidgets)
{
	// With an input group, as the simulator's keyboard and mouse wheel have, every button joins it
	lv_group_t* group = lv_group_create();
	lv_group_set_default(group);
	static int focusChanges;
	lv_group_set_focus_cb(group, [](lv_group_t*) { focusChanges++; });
	UI::Xml::reload();
	ASSERT_GT(lv_group_get_obj_count(group), 1u);

	// Deleting the focused widget used to move focus to the next one to be deleted, and so on through the whole UI,
	// laying out the half-deleted screen each time; a layout that never settled hung the reload
	focusChanges = 0;
	UI::Xml::reload();
	EXPECT_EQ(focusChanges, 1) << "only the rebuilt UI's first widget is focused";

	lv_group_set_default(nullptr);
	lv_group_delete(group);
}

TEST_F(TestHomeViewWithData, TabsBuiltDuringAJobSelectAnEnabledTab)
{
	// Built while a job runs, the dashboard's Jobs tab is disabled from the start
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	UI::Xml::reload();

	auto& tabs = dashboardTabs();
	EXPECT_TRUE(tabs.isTabDisabled(0));
	EXPECT_EQ(tabs.getActiveTabIndex(), STATUS_TAB) << "the tab the job start selected, kept by the reload";
}

TEST_F(TestHomeView, ResponsesAreShownOneAtATime)
{
	lv_obj_t* response = notificationPart("response");
	notifications().newResponse(ResponseType::INFO, "first");
	notifications().newResponse(ResponseType::WARNING, "second");
	ASSERT_FALSE(lv_obj_has_flag(response, LV_OBJ_FLAG_HIDDEN));
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(response, "text")), "first");

	lv_obj_send_event(lv_obj_find_by_name(response, "cancel"), LV_EVENT_CLICKED, nullptr);
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(response, "text")), "second");

	lv_obj_send_event(lv_obj_find_by_name(response, "cancel"), LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(lv_obj_has_flag(response, LV_OBJ_FLAG_HIDDEN));

	// The console shows responses itself
	UI::openScreen(std::string_view("console"), false);
	notifications().newResponse(ResponseType::INFO, "in the console");
	EXPECT_TRUE(lv_obj_has_flag(response, LV_OBJ_FLAG_HIDDEN));
}

TEST_F(TestHomeView, AlertAnswersThePrinter)
{
	std::vector<std::string> sent;
	UI::Xml::Services::setGcodeSender([&](std::string_view gcode) { sent.emplace_back(gcode); });
	lv_obj_t* response = notificationPart("response");
	auto alertPart = [](const char* name) { return lv_obj_find_by_name(notificationPart("alert"), name); };
	auto click = [&](const char* name) { lv_obj_send_event(alertPart(name), LV_EVENT_CLICKED, nullptr); };

	// A response waits while an alert is open
	load_model_data_from_file("tests/object_model/m291/s3.json");
	notifications().newAlertData(OM::g_currentAlert);
	notifications().newResponse(ResponseType::INFO, "waiting");
	EXPECT_TRUE(lv_obj_has_flag(response, LV_OBJ_FLAG_HIDDEN));

	// Cancel, offered by the printer, answers it
	const uint32_t seq3 = OM::g_currentAlert.seq;
	click("cancel");
	EXPECT_EQ(sent, (std::vector<std::string>{fmt::format("M292 P1 S{:d}\n", seq3)}));
	EXPECT_TRUE(lv_obj_has_flag(notificationPart("alert"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(response, LV_OBJ_FLAG_HIDDEN)) << "then the response shows";
	lv_obj_send_event(lv_obj_find_by_name(response, "cancel"), LV_EVENT_CLICKED, nullptr);

	// A choice answers with its index
	sent.clear();
	load_model_data_from_file("tests/object_model/m291/s4.json");
	notifications().newAlertData(OM::g_currentAlert);
	lv_obj_t* choices = lv_obj_find_by_name(notificationPart("choices_list"), "list");
	ASSERT_EQ(lv_obj_get_child_count(choices), 3u);
	lv_obj_send_event(lv_obj_get_child(choices, 1), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(sent, (std::vector<std::string>{fmt::format("M292 R{{1}} S{:d}\n", OM::g_currentAlert.seq)}));

	// A value can't be sent until it is within the limits
	sent.clear();
	load_model_data_from_file("tests/object_model/m291/s5_limits.json");
	notifications().newAlertData(OM::g_currentAlert);
	alertInput().setText("11");
	EXPECT_TRUE(lv_obj_has_state(alertPart("ok"), LV_STATE_DISABLED));
	alertInput().setText("7");
	EXPECT_FALSE(lv_obj_has_state(alertPart("ok"), LV_STATE_DISABLED));
	click("ok");
	EXPECT_EQ(sent, (std::vector<std::string>{fmt::format("M292 R{{7}} S{:d}\n", OM::g_currentAlert.seq)}));
	UI::Xml::Services::setGcodeSender(nullptr);
}

namespace
{
	/** Run the Model's queued events, as the main loop would */
	void runEvents()
	{
		Model::get().startEventLoop();
		Model::get().stopEventLoop();
	}

	/** Whether `obj` and every widget it is in are shown */
	bool shownOnScreen(lv_obj_t* obj)
	{
		for (; obj != nullptr; obj = lv_obj_get_parent(obj))
		{
			if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN))
			{
				return false;
			}
		}
		return true;
	}

	/** The shared number pad's dialog if it is open, else nullptr */
	lv_obj_t* openNumberPad()
	{
		lv_obj_t* dialog = lv_obj_find_by_name(lv_screen_active(), "numberpad_dialog");
		return dialog != nullptr && shownOnScreen(dialog) ? dialog : nullptr;
	}

	/**
	 * Tap the widget called `name` (in the widget called `within`, if given) in the view registered as `view`, and
	 * say whether a number pad was shown. `sliderValue` taps the value of the slider called `name`.
	 */
	bool tapShowsNumberPad(const char* view, const char* within, const char* name, bool sliderValue = false)
	{
		UI::closeAllModals();
		lv_obj_t* obj = UI::findView(view)->getRootPtr();
		if (within != nullptr)
		{
			obj = lv_obj_find_by_name(obj, within);
		}
		obj = obj == nullptr ? nullptr : lv_obj_find_by_name(obj, name);
		if (obj == nullptr)
		{
			ADD_FAILURE() << view << " has no " << name;
			return false;
		}
		if (sliderValue)
		{
			obj = static_cast<UI::Slider&>(*UI::LvObj::fromPtr(obj)).getInput().getRootPtr();
		}
		lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
		return openNumberPad() != nullptr;
	}
} // namespace

TEST_F(TestHomeViewWithData, EveryNumberPadOpens)
{
	runEvents(); // Fill the lists from the object model
	{
		lv_obj_t* toolList = UI::findView("dashboard_tools")->getRootPtr();
		lv_obj_t* firstHeater = lv_obj_find_by_name(lv_obj_find_by_name(toolList, "tools"), "heaters");
		lv_obj_send_event(lv_obj_find_by_name(firstHeater, "active"), LV_EVENT_CLICKED, nullptr);
		EXPECT_NE(openNumberPad(), nullptr) << "dashboard heater";
	}

	openControlTab(0);
	EXPECT_TRUE(tapShowsNumberPad("move", "z_control", "label")) << "move Z position";
	openControlTab(1);
	EXPECT_TRUE(tapShowsNumberPad("temperature", nullptr, "active_temperature")) << "temperature heater";
	EXPECT_TRUE(tapShowsNumberPad("temperature", nullptr, "value")) << "value selector";
	openControlTab(4);
	EXPECT_TRUE(tapShowsNumberPad("fans", nullptr, "slider", true)) << "fan slider";

	UI::openScreen(std::string_view("settings"), false);
	settingsPage().setActiveTab(0);
	EXPECT_TRUE(tapShowsNumberPad("settings", nullptr, "brightness", true)) << "brightness";
	settingsPage().setActiveTab(1);
	EXPECT_TRUE(tapShowsNumberPad("settings", nullptr, "poll_interval", true)) << "poll interval";
	EXPECT_TRUE(tapShowsNumberPad("settings", nullptr, "duet_ip_address")) << "IP address";
	settingsPage().setActiveTab(2);
	EXPECT_TRUE(tapShowsNumberPad("settings", nullptr, "chroma_slider", true)) << "chroma";
	UI::closeAllModals();
}

TEST_F(TestHomeViewWithData, TemperatureTabOpensTheExtrusionFactorDialog)
{
	runEvents();
	openControlTab(1);
	lv_obj_t* factors = lv_obj_find_by_name(UI::findView("temperature")->getRootPtr(), "extrusion_factors");
	ASSERT_NE(factors, nullptr);
	ASSERT_GT(lv_obj_get_child_count(factors), 0u);
	lv_obj_t* firstFactor = lv_obj_get_child(lv_obj_find_by_name(factors, "list"), 0);
	ASSERT_NE(firstFactor, nullptr);
	lv_obj_send_event(firstFactor, LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/extrusion_factor_dialog.png")
	UI::closeAllModals();
}

TEST_F(TestHomeViewWithData, ViewsInsidePlainContainersComeAliveWithTheirPage)
{
	// The extruder panel is inside a plain <lv_obj> of the Temperature tab, which has nothing to pass a show on
	auto* extruder = UI::findView<UI::Xml::XmlView>("extruder_control");
	ASSERT_NE(extruder, nullptr);
	ASSERT_NE(extruder->getPresenter(), nullptr);
	EXPECT_FALSE(extruder->getPresenter()->isActive()) << "the Control page isn't open yet";

	openControlTab(1);
	EXPECT_TRUE(extruder->getPresenter()->isActive());

	UI::home();
	EXPECT_FALSE(extruder->getPresenter()->isActive()) << "nothing listens while its page is closed";
}

TEST_F(TestHomeViewWithData, NumberPadAnswersTheSliderAndCloses)
{
	UI::openScreen(std::string_view("settings"), false);
	settingsPage().setActiveTab(1);
	ASSERT_TRUE(tapShowsNumberPad("settings", nullptr, "poll_interval", true));
	lv_obj_t* dialog = openNumberPad();

	// Typed on the keys, then OK
	lv_textarea_set_text(lv_obj_find_by_name(dialog, "text"), "1234");
	lv_obj_t* keys = lv_obj_find_by_name(dialog, "keys");
	lv_obj_send_event(keys, LV_EVENT_READY, nullptr);
	EXPECT_EQ(openNumberPad(), nullptr) << "confirming closes it";
	auto& slider = static_cast<UI::Slider&>(
		*UI::LvObj::fromPtr(lv_obj_find_by_name(page<UI::LvObj>("settings").getRootPtr(), "poll_interval")));
	EXPECT_EQ(slider.getValue(), 1234.0f);

	// Out of range: OK does nothing
	ASSERT_TRUE(tapShowsNumberPad("settings", nullptr, "poll_interval", true));
	lv_textarea_set_text(lv_obj_find_by_name(dialog, "text"), "5");
	lv_obj_send_event(keys, LV_EVENT_READY, nullptr);
	EXPECT_NE(openNumberPad(), nullptr) << "a value it can't take isn't confirmed";
	EXPECT_EQ(slider.getValue(), 1234.0f);
	UI::closeAllModals();
}

TEST_F(TestHomeViewWithData, NumberPadQuestionGoesWithItsSlider)
{
	// The fans go while one's question is open, taking its slider: the question goes, and nothing answers the slider
	runEvents();
	openControlTab(4);
	ASSERT_TRUE(tapShowsNumberPad("fans", nullptr, "slider", true));
	OM::RemoveFan(0, true);
	Model::get().post<EventType::FanData>();
	runEvents();
	EXPECT_EQ(openNumberPad(), nullptr);
}

TEST_F(TestHomeViewWithData, FactorDialogTypesTheTappedFactor)
{
	std::vector<std::string> sent;
	UI::Xml::Services::setGcodeSender([&sent](std::string_view line) { sent.emplace_back(line); });
	runEvents();
	dashboardTabs().setActiveTab(STATUS_TAB);
	lv_obj_send_event(lv_obj_find_by_name(page<UI::LvObj>("dashboard_job_status").getRootPtr(), "flow_multiplier"),
					  LV_EVENT_CLICKED,
					  nullptr);
	lv_obj_t* dialog = lv_obj_find_by_name(lv_screen_active(), "extrusion_dialog");
	ASSERT_NE(dialog, nullptr);
	ASSERT_TRUE(shownOnScreen(dialog));
	lv_obj_t* pad = lv_obj_find_by_name(dialog, "numberpad");
	EXPECT_TRUE(shownOnScreen(pad)) << "the number pad starts on the first extruder";
	EXPECT_EQ(labelText(pad, "header"), "Extruder 0");

	// Tap the second extruder's value: the pad is for it now
	lv_obj_t* rows = lv_obj_find_by_name(dialog, "list");
	ASSERT_EQ(lv_obj_get_child_count(rows), 2u);
	auto& second = static_cast<UI::Slider&>(*UI::LvObj::fromPtr(lv_obj_get_child(rows, 1)));
	lv_obj_send_event(second.getInput().getRootPtr(), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQ(labelText(pad, "header"), "Extruder 1");

	// Type 95 and confirm: sent, and the dialog closes
	lv_textarea_set_text(lv_obj_find_by_name(pad, "text"), "95");
	lv_obj_send_event(lv_obj_find_by_name(pad, "keys"), LV_EVENT_READY, nullptr);
	ASSERT_EQ(sent.size(), 1u);
	EXPECT_EQ(sent[0], "M221 D1 S95\n");
	EXPECT_FALSE(shownOnScreen(dialog));
	UI::Xml::Services::setGcodeSender(nullptr);
}

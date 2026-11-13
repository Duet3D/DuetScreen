/*
 * test_home_view.cpp
 *
 *  Created on: 2025-08-14
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/PrinterStatus.h"
#include "Storage.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Console/ConsoleView.h"
#include "UI/Screens/Control/ControlView.h"
#include "UI/Screens/Dashboard/DashboardTabs.h"
#include "UI/Screens/Notifications/NotificationsView.h"
#include "UI/Screens/Settings/SettingsView.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/TemperatureSource.h"
#include "UI/Xml/View/XmlView.h"
#include "UI/Xml/Widgets/DsGraph.h"
#include "helix_xml.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include "utils/UpgradeHelper.h"
#include <gtest/gtest.h>

class TestHomeView : public UiTestSuite
{
  protected:
	/** The home screen, as the application shows it */
	TestHomeView()
		: home("home", lv_screen_active(), "home_screen")
	{
		ZoneScoped;
		home.show();
	}

	/** The view home_screen.xml registers as `name`; fails the test if there is none */
	template <typename T>
	static T& page(std::string_view name)
	{
		T* view = UI::findView<T>(name);
		if (view == nullptr)
		{
			throw std::runtime_error(fmt::format("home_screen has no {:s}", name));
		}
		return *view;
	}

	static UI::DashboardTabs& dashboardTabs() { return page<UI::DashboardTabs>("dashboard_tabs"); }

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

	static UI::ControlView& controlPage() { return page<UI::ControlView>("control"); }
	static UI::ConsoleView& consolePage() { return page<UI::ConsoleView>("console"); }

	/** The Files page's tabs, as files_screen.xml builds them */
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
			lv_obj_t* dialog = lv_obj_find_by_name(backdrop, "file_dialog");
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
	static UI::SettingsView& settingsPage() { return page<UI::SettingsView>("settings"); }
	static UI::NotificationsView& notificationsPage() { return page<UI::NotificationsView>("notifications"); }

	UI::Xml::XmlView home;
};

TEST_F(TestHomeView, BlankView)
{
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/blank.png")

	notificationsPage().showKeyboard(true);
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/keyboard.png");
}

TEST_F(TestHomeView, BlankControlView)
{
	auto& control = controlPage();

	openScreen(&control);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_blank.png");

	control.showTemperatureView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/temperature_blank.png");

	control.showHeightmapView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/heightmap_blank.png");

	control.showObjectCancelView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel_blank.png");

	control.showFanView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/fan_blank.png");

	control.showMoveView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_blank.png");
}

TEST_F(TestHomeView, ObjectCancelCanvasTitleTracksCurrentObject)
{
	auto& control = controlPage();
	openScreen(&control);
	control.showObjectCancelView();

	auto& objectCancel = control.getObjectCancelView();

	objectCancel.setCurrentObjectName("gear (Instance 5)");
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel/set_object_name.png");

	objectCancel.setCurrentObjectName("");
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel/clear_object_name.png");
}

TEST_F(TestHomeView, ObjectCancelCurrentButtonRespectsConfirmationSetting)
{
	auto& control = controlPage();
	openScreen(&control);
	control.showObjectCancelView();

	auto& objectCancel = control.getObjectCancelView();
	auto& modal = objectCancel.getConfirmModal();

	OM::ClearJobObjects();
	auto object = OM::GetOrCreateJobObject(0);
	ASSERT_NE(object, nullptr);
	object->index = 0;
	object->name = "gear (Instance 1)";
	OM::SetCurrentJobObject(0);

	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, true);
	objectCancel.getCancelCurrentButton().sendEvent(LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(modal.isVisible());
	EXPECT_EQ(std::string(modal.getTitle().getText()), _("object_cancel.confirm_cancel_current_title"));
	EXPECT_NE(std::string(modal.getText().getText()).find("gear (Instance 1)"), std::string::npos);
	modal.close();

	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, false);
	objectCancel.getCancelCurrentButton().sendEvent(LV_EVENT_CLICKED, nullptr);
	EXPECT_FALSE(modal.isVisible());

	StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, true);
	OM::SetCurrentJobObject(-1);
	OM::ClearJobObjects();
}

TEST_F(TestHomeView, BlankConsoleView)
{
	openScreen(&consolePage(), false);
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
	dashboardTabs().showStatusTab();
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/blank.png")
}

TEST_F(TestHomeView, SidebarOpensPagesAndShowsWhichIsOpen)
{
	lv_obj_t* sidebar = UI::findView("side_bar")->getRootPtr();
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
	notificationsPage().getPresenter()->newResponse(ResponseType::INFO, "This is a response message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response.png");
}

TEST_F(TestHomeView, SuccessResponse)
{
	notificationsPage().getPresenter()->newResponse(ResponseType::SUCCESS, "This is a success message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_success.png");
}

TEST_F(TestHomeView, WarningResponse)
{
	notificationsPage().getPresenter()->newResponse(ResponseType::WARNING, "This is a warning message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_warning.png");
}

TEST_F(TestHomeView, ErrorResponse)
{
	notificationsPage().getPresenter()->newResponse(ResponseType::ERROR, "This is an error message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_error.png");
}

TEST_F(TestHomeView, HandleUpdateResultSuccess)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::Success,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v1.0.0",
									.updateBuildrootVersion = "v1.0.0"};

	notificationsPage().getPresenter()->handleUpdateResult(info);

	auto& prompt = notificationsPage().getUpdatePrompt();
	EXPECT_TRUE(prompt.isVisible());
	EXPECT_TRUE(prompt.getOkBtn().isVisible());
	EXPECT_FALSE(prompt.getCancelBtn().isVisible());
	EXPECT_NE(std::string(prompt.getTitle().getText()), "");
	EXPECT_NE(std::string(prompt.getText().getText()), "");

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/update_success.png");
	prompt.close();
}

TEST_F(TestHomeView, HandleUpdateResultBuildrootError)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::BuildrootVersionError,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v0.1.0",
									.updateBuildrootVersion = "v1.0.0"};

	notificationsPage().getPresenter()->handleUpdateResult(info);

	auto& prompt = notificationsPage().getUpdatePrompt();
	EXPECT_TRUE(prompt.isVisible());
	EXPECT_TRUE(prompt.getOkBtn().isVisible());
	EXPECT_FALSE(prompt.getCancelBtn().isVisible());
	EXPECT_NE(std::string(prompt.getTitle().getText()), "");
	EXPECT_NE(std::string(prompt.getText().getText()), "");

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/update_failed.png");
	prompt.close();
}

TEST_F(TestHomeView, HandleUpdateResultBuildrootWarning)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::BuildrootVersionWarning,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v1.0.0",
									.updateBuildrootVersion = "v1.0.1"};

	notificationsPage().getPresenter()->handleUpdateResult(info);

	auto& prompt = notificationsPage().getUpdatePrompt();
	EXPECT_TRUE(prompt.isVisible());
	EXPECT_TRUE(prompt.getOkBtn().isVisible());
	EXPECT_FALSE(prompt.getCancelBtn().isVisible());
	EXPECT_NE(std::string(prompt.getTitle().getText()), "");
	EXPECT_NE(std::string(prompt.getText().getText()), "");

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/update_warning.png");
	prompt.close();
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
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s0.png");
}

TEST_F(TestHomeView, AlertS1)
{
	load_model_data_from_file("tests/object_model/m291/s1.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s1.png");
}

TEST_F(TestHomeView, AlertS2)
{
	load_model_data_from_file("tests/object_model/m291/s2.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s2.png");
}

TEST_F(TestHomeView, AlertS3)
{
	load_model_data_from_file("tests/object_model/m291/s3.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s3.png");

	load_model_data_from_file("tests/object_model/test_bench/model_move_vn.json");
	load_model_data_from_file("tests/object_model/m291/s3_axis.json");
	home.show();
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s3_axis.png");
}

TEST_F(TestHomeView, AlertS4)
{
	load_model_data_from_file("tests/object_model/m291/s4.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s4.png");

	load_model_data_from_file("tests/object_model/m291/s4_j1.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s4_j1.png");
}

TEST_F(TestHomeView, AlertS5)
{
	load_model_data_from_file("tests/object_model/m291/s5.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5.png");

	load_model_data_from_file("tests/object_model/m291/s5_j1.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_j1.png");

	load_model_data_from_file("tests/object_model/m291/s5_limits.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits.png");

	auto input_obj = notificationsPage().getChildByName("modal_bg.alert.body.input_cont.input");
	ASSERT_NE(input_obj, nullptr);
	input_obj->sendEvent(LV_EVENT_FOCUSED);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits_input.png");

	auto& input = static_cast<UI::LvTextarea&>(*input_obj);
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
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6.png");

	load_model_data_from_file("tests/object_model/m291/s6_j1.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_j1.png");

	load_model_data_from_file("tests/object_model/m291/s6_limits.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits.png");

	auto input_obj = notificationsPage().getChildByName("modal_bg.alert.body.input_cont.input");
	ASSERT_NE(input_obj, nullptr);
	input_obj->sendEvent(LV_EVENT_FOCUSED);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits_input.png");

	auto& input = static_cast<UI::LvTextarea&>(*input_obj);
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
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_j1.png");

	load_model_data_from_file("tests/object_model/m291/s7_limits.json");
	notificationsPage().getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits.png");

	auto input_obj = notificationsPage().getChildByName("modal_bg.alert.body.input_cont.input");
	ASSERT_NE(input_obj, nullptr);
	input_obj->sendEvent(LV_EVENT_FOCUSED);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits_input.png");

	auto& input = static_cast<UI::LvTextarea&>(*input_obj);
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
	// The first tool's first heater row, as tool_list.xml builds it
	lv_obj_t* toolList = UI::findView("tool_list")->getRootPtr();
	lv_obj_t* firstHeater = lv_obj_find_by_name(lv_obj_find_by_name(toolList, "tools"), "heaters");
	lv_obj_send_event(lv_obj_find_by_name(firstHeater, "active"), LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/tool_list_numberpad.png");

	UI::closeAllModals();

	/* Test all themes */
	for (auto& [name, theme] : UI::Themes::getThemes())
	{
		theme->setThemeActive();
		EXPECT_EQUAL_SCREENSHOT(fmt::format("home_view/dashboard/theme_{:s}.png", theme->getName()).c_str());
	}
}

TEST_F(TestHomeViewWithData, ToolListLocksHeatersWhilePrinting)
{
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	Model::get().startEventLoop();
	Model::get().stopEventLoop();
	lv_obj_t* toolList = UI::findView("tool_list")->getRootPtr();
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
	openScreen(&consolePage());

	auto presenter = consolePage().getPresenter();
	consolePage().addCommand("M115");
	presenter->newResponse(ResponseType::INFO, "Testing response");
	presenter->newResponse(ResponseType::INFO, "new multi-line response\nline 2");
	presenter->newResponse(ResponseType::SUCCESS, "Success response");
	presenter->newResponse(ResponseType::WARNING, "Warning response");
	presenter->newResponse(ResponseType::ERROR, "Error response");

	presenter->newLogMessage(Log::DebugLevel::Info, Log::log_time_t{}, "Testing log message");

	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/duet_responses.png")

	consolePage().showCommandList(true, false);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/command_list.png");

	consolePage().showCommandList(false, false);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/command_list_hidden.png");

	consolePage().showKeyboard(true);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/keyboard.png");
}

TEST_F(TestHomeViewWithData, ControlView)
{
	ZoneScoped;
	auto& control = controlPage();

	openScreen(&control);
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move.png");

	load_model_data_from_file("tests/object_model/test_bench/model_10_axes.json");
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_10_axes.png");
	{
		auto btn = control.getMoveView().getChildByName(
			"central_row.babystep_cont.babystep.button_panel.value_list.list.value_btn_1");
		ASSERT_NE(btn, nullptr);

		btn->sendEvent(LV_EVENT_LONG_PRESSED);
		EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_babystep_long_press.png");
	}

	control.showTemperatureView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/temperature.png");
	{
		auto& temperature = control.getTemperatureView();
		auto input =
			temperature.getChildByName("control_cont.extruder_control.controls.distance_selector.topRow.valueDisplay");
		ASSERT_NE(input, nullptr);
		input->sendEvent(LV_EVENT_CLICKED, nullptr);
		EXPECT_EQUAL_SCREENSHOT("home_view/control_view/temperature_distance_input.png");
	}

	control.showHeightmapView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/heightmap.png");

	control.showFanView();
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
	control.showObjectCancelView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/object_cancel.png");

	control.showMoveView();
	EXPECT_EQUAL_SCREENSHOT("home_view/control_view/move_10_axes.png");
}

TEST_F(TestHomeViewWithData, MacroView)
{
	openFilesTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/macros.png")
}

TEST_F(TestHomeViewWithData, SettingsView)
{
	auto& settings = settingsPage();
	openScreen(&settings);
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/initial.png")

	settings.showGeneralSettings();
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/general.png")

	settings.showConnectionSettings();
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait for wifi networks to load
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/connection.png")

	settings.showDisplaySettings();
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/display.png")

	settings.showDeveloperSettings();
	EXPECT_EQUAL_SCREENSHOT("home_view/settings_view/developer.png")
}

TEST_F(TestHomeViewWithData, StatusView)
{
	dashboardTabs().showStatusTab();
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/printing.png")

	lv_obj_t* status = page<UI::LvObj>("dashboard_tabs").getRootPtr();
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

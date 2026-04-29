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
#include "UI/Screens/File/FileView.h"
#include "UI/Screens/Home/HomeView.h"
#include "i18n/i18n.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include "utils/UpgradeHelper.h"
#include <gtest/gtest.h>

class TestHomeView : public UiTestSuite
{
  protected:
	TestHomeView()
	{
		ZoneScoped;
		// Override the singleton so code using HomeView::instance() uses our local view
		UI::HomeView::setInstance(&view);
	}

	virtual ~TestHomeView()
	{
		// Reset override
		UI::HomeView::setInstance(nullptr);
	}

	UI::HomeView view;
};

TEST_F(TestHomeView, BlankView)
{
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/blank.png")

	view.showKeyboard(true);
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/keyboard.png");
}

TEST_F(TestHomeView, BlankControlView)
{
	auto& control = view.getControlView();

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
	auto& control = view.getControlView();
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
	auto& control = view.getControlView();
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
	openScreen(&view.getConsoleView(), false);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/blank.png")
}

TEST_F(TestHomeView, BlankMacroView)
{
	auto& files = view.getFileView();
	openScreen(&files, false);
	files.setActiveTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/macros_blank.png");
}

TEST_F(TestHomeView, BlankJobView)
{
	auto& files = view.getFileView();
	openScreen(&files, false);
	files.setActiveTab(1);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/jobs_blank.png");
}

TEST_F(TestHomeView, JobModalShowsDeleteButton)
{
	auto& files = view.getFileView();
	openScreen(&files, false);
	files.setActiveTab(1);

	auto* tab = files.getTab(1);
	ASSERT_NE(tab, nullptr);
	auto& jobsView = static_cast<UI::FileView&>(*tab->getChild(0));

	jobsView.confirmStartPrint("test.gcode", std::filesystem::path{});
	auto& modal = jobsView.getConfirmModal();

	EXPECT_TRUE(modal.isVisible());
	auto* deleteBtn = modal.getChildByName("footer.delete");
	ASSERT_NE(deleteBtn, nullptr);
	EXPECT_TRUE(deleteBtn->isVisible());
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/start_print_modal.png");

	modal.cancel();
}

TEST_F(TestHomeView, MacroModalHidesDeleteButton)
{
	auto& files = view.getFileView();
	openScreen(&files, false);
	files.setActiveTab(0);

	auto* tab = files.getTab(0);
	ASSERT_NE(tab, nullptr);
	auto& macrosView = static_cast<UI::FileView&>(*tab->getChild(0));

	macrosView.confirmRunMacro("test.g");
	auto& modal = macrosView.getConfirmModal();

	EXPECT_TRUE(modal.isVisible());
	auto* deleteBtn = modal.getChildByName("footer.delete");
	ASSERT_NE(deleteBtn, nullptr);
	EXPECT_FALSE(deleteBtn->isVisible());
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/run_macro_modal.png");

	modal.cancel();
}

TEST_F(TestHomeView, DeleteButtonOpensDeleteConfirmationModal)
{
	auto& files = view.getFileView();
	openScreen(&files, false);
	files.setActiveTab(1);

	auto* tab = files.getTab(1);
	ASSERT_NE(tab, nullptr);
	auto& jobsView = static_cast<UI::FileView&>(*tab->getChild(0));

	jobsView.confirmStartPrint("test.gcode", std::filesystem::path{});
	auto& modal = jobsView.getConfirmModal();

	auto* deleteBtn = modal.getChildByName("footer.delete");
	ASSERT_NE(deleteBtn, nullptr);
	ASSERT_TRUE(deleteBtn->isVisible());

	deleteBtn->sendEvent(LV_EVENT_CLICKED, nullptr);

	EXPECT_EQ(std::string(modal.getTitle().getText()), "Delete File");
	EXPECT_EQ(std::string(modal.getText().getText()), "Are you sure you want to delete test.gcode?");
	EXPECT_FALSE(deleteBtn->isVisible());
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/confirm_delete_modal.png");

	modal.cancel();
}

TEST_F(TestHomeView, BlankStatusView)
{
	view.getDashboard().showStatusTab();
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/blank.png")
}

#if SIDE_BAR_APP_DRAWER
TEST_F(TestHomeView, BlankMoveView)
{
	openScreen(&view.getMoveView(), false);
	EXPECT_EQUAL_SCREENSHOT("home_view/move_view/blank.png");
}

TEST_F(TestHomeView, BlankTemperatureView)
{
	openScreen(&view.getTemperatureView(), false);
	EXPECT_EQUAL_SCREENSHOT("home_view/temperature_view/blank.png")
}

TEST_F(TestHomeView, BlankFanView)
{
	openScreen(&view.getFanView(), false);
	EXPECT_EQUAL_SCREENSHOT("home_view/fan_view/blank.png");
}

TEST_F(TestHomeView, BlankHeightmapView)
{
	openScreen(&view.getHeightmapView(), false);
	EXPECT_EQUAL_SCREENSHOT("home_view/heightmap_view/blank.png");
}

TEST_F(TestHomeView, AppDrawer)
{
	view.show();
	view.getSideBar().showAppDrawer(true, LV_ANIM_OFF);
	EXPECT_EQUAL_SCREENSHOT("home_view/app_drawer.png");
}
#endif

TEST_F(TestHomeView, Response)
{
	view.getPresenter()->newResponse(ResponseType::INFO, "This is a response message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response.png");
}

TEST_F(TestHomeView, SuccessResponse)
{
	view.getPresenter()->newResponse(ResponseType::SUCCESS, "This is a success message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_success.png");
}

TEST_F(TestHomeView, WarningResponse)
{
	view.getPresenter()->newResponse(ResponseType::WARNING, "This is a warning message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_warning.png");
}

TEST_F(TestHomeView, ErrorResponse)
{
	view.getPresenter()->newResponse(ResponseType::ERROR, "This is an error message from the Duet");
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/response_error.png");
}

TEST_F(TestHomeView, HandleUpdateResultSuccess)
{
	UpgradeHelper::UpgradeInfo info{.result = UpgradeHelper::UpgradeResult::Success,
									.currentVersion = "v1.2.3",
									.currentBuildrootVersion = "v1.0.0",
									.updateBuildrootVersion = "v1.0.0"};

	view.getPresenter()->handleUpdateResult(info);

	auto& prompt = view.getUpdatePrompt();
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

	view.getPresenter()->handleUpdateResult(info);

	auto& prompt = view.getUpdatePrompt();
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

	view.getPresenter()->handleUpdateResult(info);

	auto& prompt = view.getUpdatePrompt();
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
	auto& files = view.getFileView();
	ASSERT_NE(files.getTabButton(1), nullptr);

	view.getPresenter()->newStatus(OM::PrinterStatus::processing);
	EXPECT_TRUE(files.getTabButton(1)->hasState(LV_STATE_DISABLED));

	view.getPresenter()->newStatus(OM::PrinterStatus::idle);
	EXPECT_FALSE(files.getTabButton(1)->hasState(LV_STATE_DISABLED));
}

TEST_F(TestHomeView, AlertS0)
{
	load_model_data_from_file("tests/object_model/m291/s0.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s0.png");
}

TEST_F(TestHomeView, AlertS1)
{
	load_model_data_from_file("tests/object_model/m291/s1.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s1.png");
}

TEST_F(TestHomeView, AlertS2)
{
	load_model_data_from_file("tests/object_model/m291/s2.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s2.png");
}

TEST_F(TestHomeView, AlertS3)
{
	load_model_data_from_file("tests/object_model/m291/s3.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s3.png");

	load_model_data_from_file("tests/object_model/test_bench/model_move_vn.json");
	load_model_data_from_file("tests/object_model/m291/s3_axis.json");
	view.show();
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s3_axis.png");
}

TEST_F(TestHomeView, AlertS4)
{
	load_model_data_from_file("tests/object_model/m291/s4.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s4.png");

	load_model_data_from_file("tests/object_model/m291/s4_j1.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s4_j1.png");
}

TEST_F(TestHomeView, AlertS5)
{
	load_model_data_from_file("tests/object_model/m291/s5.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5.png");

	load_model_data_from_file("tests/object_model/m291/s5_j1.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_j1.png");

	load_model_data_from_file("tests/object_model/m291/s5_limits.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s5_limits.png");

	auto input_obj = view.getChildByName("modal_bg.alert.body.input_cont.input");
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
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6.png");

	load_model_data_from_file("tests/object_model/m291/s6_j1.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_j1.png");

	load_model_data_from_file("tests/object_model/m291/s6_limits.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s6_limits.png");

	auto input_obj = view.getChildByName("modal_bg.alert.body.input_cont.input");
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
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_j1.png");

	load_model_data_from_file("tests/object_model/m291/s7_limits.json");
	view.getPresenter()->newAlertData(OM::g_currentAlert);
	EXPECT_EQUAL_SCREENSHOT("home_view/m291/s7_limits.png");

	auto input_obj = view.getChildByName("modal_bg.alert.body.input_cont.input");
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

		view.show();
	}
};

TEST_F(TestHomeViewWithData, Dashboard)
{
	ZoneScoped;
	EXPECT_EQ(OM::Heat::GetHeaterCount(), 4);
	EXPECT_EQ(OM::GetToolCount(), 4);

	/* Populate graph with fake sensor data */
	auto sensor = OM::GetAnalogSensorBySlot(0);
	sensor->lastReading = 25.0f;
	for (size_t i = 0; i < 1000; i++)
	{
		view.getDashboard().getGraph().getPresenter()->tick();
		sensor->lastReading = (int32_t)(sensor->lastReading + 1) % 300;
	}

	view.getDashboard().getGraph().showSeries(1, false);

	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/temperature_graph.png");

	/* Open the tool list numberpad */
	view.getDashboard().getToolList().getTool(0)->getHeater(0)->getChildByName("active")->sendEvent(LV_EVENT_CLICKED,
																									nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/dashboard/tool_list_numberpad.png");

	UI::closeAllModals();

	/* Test all themes */
	for (auto& [name, theme] : UI::Themes::getThemes())
	{
		theme->setThemeActive();
		EXPECT_EQUAL_SCREENSHOT(fmt::format("home_view/dashboard/theme_{:s}.png", theme->getName()).c_str());
	}
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
	openScreen(&view.getConsoleView());

	auto presenter = view.getConsoleView().getPresenter();
	view.getConsoleView().addCommand("M115");
	presenter->newResponse(ResponseType::INFO, "Testing response");
	presenter->newResponse(ResponseType::INFO, "new multi-line response\nline 2");
	presenter->newResponse(ResponseType::SUCCESS, "Success response");
	presenter->newResponse(ResponseType::WARNING, "Warning response");
	presenter->newResponse(ResponseType::ERROR, "Error response");

	presenter->newLogMessage(Log::DebugLevel::Info, Log::log_time_t{}, "Testing log message");

	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/duet_responses.png")

	view.getConsoleView().showCommandList(true, false);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/command_list.png");

	view.getConsoleView().showCommandList(false, false);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/command_list_hidden.png");

	view.getConsoleView().showKeyboard(true);
	EXPECT_EQUAL_SCREENSHOT("home_view/console_view/keyboard.png");
}

TEST_F(TestHomeViewWithData, ControlView)
{
	ZoneScoped;
	auto& control = view.getControlView();

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
	auto& files = view.getFileView();
	openScreen(&files, false);
	files.setActiveTab(0);
	EXPECT_EQUAL_SCREENSHOT("home_view/files_view/macros.png")
}

TEST_F(TestHomeViewWithData, SettingsView)
{
	auto& settings = view.getSettingsView();
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
	view.getDashboard().showStatusTab();
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/printing.png")

	auto& statusView = view.getDashboard().getStatusView();

	UI::LvObj* speed_factor = statusView.getChildByName("print_info.speed_cont.speed_multiplier");
	ASSERT_NE(speed_factor, nullptr);
	speed_factor->sendEvent(LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/speed_factor_numberpad.png")

	UI::closeAllModals();

	UI::LvObj* extrusion_factor = statusView.getChildByName("print_info.flow_cont.flow_multiplier");
	ASSERT_NE(extrusion_factor, nullptr);
	extrusion_factor->sendEvent(LV_EVENT_CLICKED, nullptr);
	OM::Move::SetExtruderFactor(0, 1.2f);
	Model::get().post<EventType::ExtruderData>();
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/extrusion_factor_numberpad.png")

	UI::closeAllModals();

	UI::LvObj* babystep = statusView.getChildByName("print_info.babystep_cont.babystep_button");
	ASSERT_NE(babystep, nullptr);
	babystep->sendEvent(LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/status_view/babystep_modal.png")
}

#if SIDE_BAR_APP_DRAWER
TEST_F(TestHomeViewWithData, MoveView)
{
	openScreen(&view.getMoveView());
	EXPECT_EQUAL_SCREENSHOT("home_view/move_view/move_view.png")

	load_model_data_from_file("tests/object_model/5_axis/model_move_vn.json");
	view.getMoveView().activate();
	EXPECT_EQUAL_SCREENSHOT("home_view/move_view/5_axis.png")
}

TEST_F(TestHomeViewWithData, TemperatureView)
{
	openScreen(&view.getTemperatureView());
	EXPECT_EQUAL_SCREENSHOT("home_view/temperature_view/temperature_view.png")
}

TEST_F(TestHomeViewWithData, FanView)
{
	openScreen(&view.getFanView());
	EXPECT_EQUAL_SCREENSHOT("home_view/fan_view/fan_view.png")
}

TEST_F(TestHomeViewWithData, HeightmapView)
{
	openScreen(&view.getHeightmapView());
	EXPECT_EQUAL_SCREENSHOT("home_view/heightmap_view/heightmap_view.png")
}

TEST_F(TestHomeViewWithData, FineTuneView)
{
	UI::FineTune& fineTuneView = view.getFineTuneView();
	openScreen(&fineTuneView);
	EXPECT_EQUAL_SCREENSHOT("home_view/fine_tune_view/fine_tune_view.png")

	UI::LvObj* input = fineTuneView.getChildByName("sliders.speed.slider_cont.slider_input");
	ASSERT_NE(input, nullptr);

	input->sendEvent(LV_EVENT_CLICKED, nullptr);
	EXPECT_EQUAL_SCREENSHOT("home_view/fine_tune_view/keyboard.png")
}
#endif

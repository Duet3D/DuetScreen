/*
 * test_status.cpp
 *
 *  Created on: 2025-08-21
 *      Author: Andy Everitt
 */

#include "Comm/Thumbnail.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestStatus : public UiTestSuite
{
  public:
	TestStatus()
		: view("status", screen, "dashboard_job_status")
	{
		// TODO: there is a race condition between the filesystem operations and the test execution when tests are run
		// in parallel

		/* Need to wait for the filesystem operations to finish fully */
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		view.setSize(LV_PCT(36), LV_PCT(90));
		home.hide();
	}

	UI::Xml::XmlView view;
	/** Hidden: the status view's modals belong to its main window */
	UI::Xml::XmlView home{"home", lv_screen_active(), "home"};
};

TEST_F(TestStatus, Blank)
{
	view.show();

	EXPECT_EQUAL_SCREENSHOT("status_view/blank.png")
}

TEST_F(TestStatus, ThumbnailGoesBesideTheNameWhereThereIsNoRoomAbove)
{
	std::filesystem::path thumbnailPath = GetThumbnailPath("test.gcode");
	std::filesystem::create_directories(thumbnailPath.parent_path());
	std::filesystem::remove(thumbnailPath);
	std::filesystem::copy_file(Themes::getFixedIconPath("examples", "print_thumbnail.png"), thumbnailPath);
	// Printing
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	load_model_data(
		R"({"key":"job","flags":"v","result":{"duration":50,"warmUpDuration":0,"file":{"fileName":"test.gcode","size":100,"printTime":100}}})");

	// Short: less room above the name than the square beside it
	view.setSize(LV_PCT(36), 340);
	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/thumbnail_beside.png");
	lv_obj_t* root = view.getRootPtr();
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(root, "picture"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(root, "beside"), LV_OBJ_FLAG_HIDDEN));

	// Tall: above the name, large
	view.setSize(LV_PCT(36), LV_PCT(90));
	view.updateLayout();
	EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(root, "picture"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(root, "beside"), LV_OBJ_FLAG_HIDDEN));

	// Short again: back beside it
	view.setSize(LV_PCT(36), 340);
	view.updateLayout();
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(root, "picture"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(root, "beside"), LV_OBJ_FLAG_HIDDEN));

	// Shorter than it all: the job's rows start at the top of its card and its details are cut off at the bottom; the
	// overrides and controls all show
	view.setSize(LV_PCT(36), 184);
	view.updateLayout();
	lv_obj_t* card = lv_obj_find_by_name(root, "job");
	EXPECT_GE(lv_obj_get_y(lv_obj_find_by_name(card, "header")), 0);
	EXPECT_EQ(lv_obj_get_scroll_bottom(root), 0);
	lv_obj_t* footer = lv_obj_find_by_name(root, "footer");
	EXPECT_LE(lv_obj_get_y(footer) + lv_obj_get_height(footer), lv_obj_get_content_height(root));
	EXPECT_EQUAL_SCREENSHOT("status_view/thumbnail_squeezed.png");
}

TEST_F(TestStatus, Header)
{
	// A job half way through (by time, the default progress source), with a thumbnail
	std::filesystem::path thumbnailPath = GetThumbnailPath("test.gcode");
	std::filesystem::create_directories(thumbnailPath.parent_path());
	std::filesystem::remove(thumbnailPath);
	std::filesystem::copy_file(Themes::getFixedIconPath("examples", "print_thumbnail.png"), thumbnailPath);
	// Printing
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	load_model_data(
		R"({"key":"job","flags":"v","result":{"duration":50,"warmUpDuration":0,"file":{"fileName":"test.gcode","size":100,"printTime":100}}})");

	view.show();
	view.updateLayout();

	EXPECT_EQUAL_SCREENSHOT("status_view/header.png")
}

TEST_F(TestStatus, PowerOn)
{
	load_model_data_from_file("tests/object_model/test_bench/model_state_vn.json");
	load_model_data_from_file("tests/object_model/job/model_job_power_on.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/power_on.png")
}

TEST_F(TestStatus, Layer2)
{
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	load_model_data_from_file("tests/object_model/job/model_job_printing_layer_2.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/layer_2.png")
}

TEST_F(TestStatus, Paused)
{
	load_model_data_from_file("tests/object_model/job/model_state_paused.json");
	load_model_data_from_file("tests/object_model/job/model_job_paused.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/paused.png")
}

TEST_F(TestStatus, Cancelling)
{
	load_model_data_from_file("tests/object_model/job/model_state_cancelling.json");
	load_model_data_from_file("tests/object_model/job/model_job_cancelling.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/cancelling.png")
}

TEST_F(TestStatus, Cancelled)
{
	load_model_data_from_file("tests/object_model/job/model_state_cancelled.json");
	load_model_data_from_file("tests/object_model/job/model_job_cancelled.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/cancelled.png")
}

/**
 * This test is currently bugged because lvgl does not calculate the size of the thumbnail image correctly in the test
 * but it does in the real UI.
 */
TEST_F(TestStatus, Thumbnail)
{
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	load_model_data_from_file("tests/object_model/job/model_job_printing_layer_2.json");

	std::string_view filename = "0:/gcodes/ROTO-VORON-HEATSINK-FAN-DUCT v4 (T0 0.6mm HF - Prusament PETG).gcode";
	std::filesystem::path thumbnailPath = GetThumbnailPath(filename);
	std::filesystem::create_directories(thumbnailPath.parent_path());
	assert(std::filesystem::exists(thumbnailPath.parent_path()));
	std::filesystem::remove(thumbnailPath);
	std::filesystem::copy_file(Themes::getFixedIconPath("examples", "print_thumbnail.png"), thumbnailPath);

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view/thumbnail.png");
}

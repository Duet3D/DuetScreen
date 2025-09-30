/*
 * test_status.cpp
 *
 *  Created on: 2025-08-21
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Screens/Status/StatusView.h"
#include "test_utils/UiTestSuite.h"
#include <gtest/gtest.h>

using namespace UI;

class TestStatus : public UiTestSuite
{
  public:
	TestStatus()
		: view(screen)
	{
		HomeView::setInstance(&home);
		view.setSize(LV_PCT(36), LV_PCT(90));
		home.hide();
	}

	~TestStatus() { HomeView::setInstance(nullptr); }

	static void SetUpTestSuite()
	{
		// TODO: there is a race condition between the filesystem operations and the test execution when tests are run
		// in parallel

		UiTestSuite::SetUpTestSuite();

		/* Need to wait for the filesystem operations to finish fully */
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		std::string_view filename = "0:/gcodes/ROTO-VORON-HEATSINK-FAN-DUCT v4 (T0 0.6mm HF - Prusament PETG).gcode";
		std::string thumbnailPath = GetThumbnailPath(filename);
		assert(CreateThumbnailDirectory(thumbnailPath));
		system(fmt::format("cp assets/examples/print_thumbnail.png '{:s}'", thumbnailPath).c_str());
	}

	StatusView view;
	HomeView home;
};

TEST_F(TestStatus, Blank)
{
	view.show();

	EXPECT_EQUAL_SCREENSHOT("status_view_blank.png")
}

TEST_F(TestStatus, Header)
{
	view.show();

	view.setFilename("test.gcode");
	view.updateProgress(50);
	view.setThumbnail(IMAGE_ASSET("example/example.bmp"));
	view.updateLayout();

	EXPECT_EQUAL_SCREENSHOT("status_view_header.png")
}

TEST_F(TestStatus, PowerOn)
{
	load_model_data_from_file("tests/object_model/test_bench/model_state_vn.json");
	load_model_data_from_file("tests/object_model/job/model_job_power_on.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view_power_on.png")
}

TEST_F(TestStatus, Layer2)
{
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	load_model_data_from_file("tests/object_model/job/model_job_printing_layer_2.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view_layer_2.png")
}

TEST_F(TestStatus, Paused)
{
	load_model_data_from_file("tests/object_model/job/model_state_paused.json");
	load_model_data_from_file("tests/object_model/job/model_job_paused.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view_paused.png")
}

TEST_F(TestStatus, Cancelling)
{
	load_model_data_from_file("tests/object_model/job/model_state_cancelling.json");
	load_model_data_from_file("tests/object_model/job/model_job_cancelling.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view_cancelling.png")
}

TEST_F(TestStatus, Cancelled)
{
	load_model_data_from_file("tests/object_model/job/model_state_cancelled.json");
	load_model_data_from_file("tests/object_model/job/model_job_cancelled.json");

	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("status_view_cancelled.png")
}

/*
 * test_job_widget.cpp
 *
 * The `job` widget (assets/ui/components/dashboard/dashboard_job.xml): the job's status while it runs, else what is
 * ready to start (job_ready.xml), from source.job.* and source.files.*.
 */

#include "Comm/Thumbnail.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "UI/Xml/Data/JobSource.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/Dialogs.h"
#include "test_utils/UiTestSuite.h"
#include <filesystem>
#include <gtest/gtest.h>
#include <thread>

class TestJobWidget : public UiTestSuite
{
  public:
	TestJobWidget()
		: view("job", screen, "dashboard_job")
	{
		/* Need to wait for the filesystem operations to finish fully */
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		view.setSize(LV_PCT(36), LV_PCT(90));
		home.hide();
	}

	/** Give the job `file` the example thumbnail */
	static void giveThumbnail(const char* file)
	{
		std::filesystem::path path = GetThumbnailPath(file);
		std::filesystem::create_directories(path.parent_path());
		std::filesystem::remove(path);
		std::filesystem::copy_file(UI::Themes::getFixedIconPath("examples", "print_thumbnail.png"), path);
	}

	/**
	 * Connect to the test bench, which lists its jobs and macros, and hand it `jobs` and `macros` as the listings; the
	 * job `thumbnailed`, if given, has the example thumbnail (kept in the printer's own folder, so once connected)
	 */
	static void connectWithFiles(const char* jobs, const char* macros, const char* thumbnailed = nullptr)
	{
		load_model_data_from_file("tests/object_model/test_bench/model_directories_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_boards_v.json");
		deliverModelEvents();
		if (thumbnailed != nullptr)
		{
			giveThumbnail(thumbnailed);
		}
		load_model_data_from_file(jobs);
		load_model_data_from_file(macros);
	}

	/** The rows of the list in the group called `group` */
	lv_obj_t* listIn(const char* group) const
	{
		return lv_obj_find_by_name(lv_obj_find_by_name(view.getRootPtr(), group), "list");
	}

	UI::Xml::XmlView view;
	/** Hidden: it hosts the shared dialogs */
	UI::Xml::XmlView home{"home", lv_screen_active(), "home"};
};

TEST_F(TestJobWidget, DurationsReadAsHoursAndMinutes)
{
	EXPECT_EQ(UI::Xml::JobSource::duration(48 * 60), "48 min");
	EXPECT_EQ(UI::Xml::JobSource::duration(73 * 60 + 10), "1 h 13 min");
	EXPECT_EQ(UI::Xml::JobSource::duration(160 * 60), "2 h 40 min");
}

TEST_F(TestJobWidget, ReadyShowsTheLastJobTheNewestJobsAndTheMacros)
{
	connectWithFiles("tests/object_model/rr_filelist/gcodes_recent.json",
					 "tests/object_model/rr_filelist/macros_dashboard.json",
					 "0:/gcodes/test.gcode");
	// A job ran, and finished while the screen watched
	load_model_data(R"({"key":"job","flags":"v","result":{"file":{"fileName":"0:/gcodes/test.gcode"}}})");
	deliverModelEvents();
	load_model_data(
		R"({"key":"job","flags":"v","result":{"file":{"fileName":null},"lastFileName":"0:/gcodes/test.gcode","lastDuration":4380,"lastFileCancelled":false,"lastFileAborted":false,"lastFileSimulated":false}})");
	deliverModelEvents();
	view.show();
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("job_widget/ready.png");
	// The macros, below
	lv_obj_t* face = lv_obj_get_parent(lv_obj_find_by_name(view.getRootPtr(), "ready_print"));
	face = lv_obj_get_parent(face); // The face, which scrolls, holds the header
	lv_obj_scroll_to_y(face, LV_COORD_MAX, LV_ANIM_OFF);
	view.updateLayout();
	EXPECT_EQUAL_SCREENSHOT("job_widget/ready_macros.png");
	lv_obj_scroll_to_y(face, 0, LV_ANIM_OFF);

	lv_obj_t* root = view.getRootPtr();
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(root, "dashboard_job_status"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(lv_obj_find_by_name(root, "last"), "detail")),
				 "Finished at 12:00 · took 1 h 13 min");

	// The five newest jobs, newest first, without the folder
	lv_obj_t* recent = listIn("recent");
	ASSERT_EQ(lv_obj_get_child_count(recent), 5u);
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(lv_obj_get_child(recent, 0), "name")), "test.gcode");
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(lv_obj_get_child(recent, 4), "name")), "Calibration cube.gcode");

	// The macros in the folder's order, named without their ordering prefix or ".g"
	lv_obj_t* macros = listIn("macros");
	ASSERT_EQ(lv_obj_get_child_count(macros), 4u);
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(lv_obj_get_child(macros, 0), "label")), "Preheat PLA");

	// Tapping a job asks whether to start it, in the shared dialog
	lv_obj_send_event(lv_obj_get_child(recent, 1), LV_EVENT_CLICKED, nullptr);
	ASSERT_NE(TestDialogs::shownNamed("file_dialog"), nullptr);
	EXPECT_EQ(TestDialogs::title(), "Start Print");
	EXPECT_EQ(TestDialogs::text("text"), "File: Cable chain link x12.gcode");
	UI::closeDialogs();

	// Tapping a macro asks whether to run it
	lv_obj_send_event(lv_obj_get_child(macros, 1), LV_EVENT_CLICKED, nullptr);
	ASSERT_NE(TestDialogs::shownNamed("file_dialog"), nullptr);
	EXPECT_EQ(TestDialogs::title(), "Run Macro");
	UI::closeDialogs();
}

TEST_F(TestJobWidget, NewFilesAreListedAsThePrinterCountsThem)
{
	connectWithFiles("tests/object_model/rr_filelist/gcodes.json", "tests/object_model/rr_filelist/macros.json");
	view.show();
	ASSERT_EQ(lv_obj_get_child_count(listIn("recent")), 1u);

	// A job uploaded: the printer counts a change to the volume's files, and the jobs are listed again
	load_model_data(R"({"key":"","flags":"d99f","result":{"seqs":{"volChanges":[27,0]}}})");
	deliverModelEvents();
	load_model_data_from_file("tests/object_model/rr_filelist/gcodes_recent.json");
	EXPECT_EQ(lv_obj_get_child_count(listIn("recent")), 5u);
}

TEST_F(TestJobWidget, ARunningJobShowsItsStatus)
{
	load_model_data_from_file("tests/object_model/job/model_state_printing.json");
	load_model_data_from_file("tests/object_model/job/model_job_printing_layer_2.json");
	deliverModelEvents();
	view.show();
	lv_obj_t* root = view.getRootPtr();
	EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(root, "dashboard_job_status"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(root, "job_ready"), LV_OBJ_FLAG_HIDDEN));
}

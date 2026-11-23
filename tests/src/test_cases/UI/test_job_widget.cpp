/*
 * test_job_widget.cpp
 *
 * The `job` widget (assets/ui/components/dashboard/dashboard_job.xml): the job's status while it runs, else what is
 * ready to start (job_ready.xml), from source.job.* and source.files.*.
 */

#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "UI/Xml/Data/JobSource.h"
#include "UI/Xml/View/XmlView.h"
#include "test_utils/Dialogs.h"
#include "test_utils/UiTestSuite.h"
#include <algorithm>
#include <filesystem>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

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

	/**
	 * Connect to the test bench, which lists its jobs and macros, and hand it `jobs` and `macros` as the listings; the
	 * job `thumbnailed`, if given, has the example thumbnail, sent by a printer the test plays (giveThumbnail())
	 */
	static void connectWithFiles(const char* jobs, const char* macros, const char* thumbnailed = nullptr)
	{
		load_model_data_from_file("tests/object_model/test_bench/model_directories_v.json");
		load_model_data_from_file("tests/object_model/test_bench/model_boards_v.json");
		deliverModelEvents();
		if (thumbnailed != nullptr)
		{
			giveThumbnail(connectScriptedPrinter(), thumbnailed);
		}
		load_model_data_from_file(jobs);
		load_model_data_from_file(macros);
	}

	/** The rows built in the windowed list `list` (only those on screen are), top first */
	static std::vector<lv_obj_t*> rowsOf(lv_obj_t* list)
	{
		std::vector<lv_obj_t*> rows;
		for (uint32_t i = 0; i < lv_obj_get_child_count(list); i++)
		{
			lv_obj_t* child = lv_obj_get_child(list, static_cast<int32_t>(i));
			if (lv_obj_find_by_name(child, "name") != nullptr)
			{
				rows.push_back(child);
			}
		}
		std::sort(rows.begin(), rows.end(), [](lv_obj_t* a, lv_obj_t* b) { return lv_obj_get_y(a) < lv_obj_get_y(b); });
		return rows;
	}

	/** The name row `row` shows */
	static std::string nameOf(lv_obj_t* row) { return lv_label_get_text(lv_obj_find_by_name(row, "name")); }

	/** A job, test.gcode, runs */
	static void runJob()
	{
		load_model_data_from_file("tests/object_model/job/model_state_printing.json");
		load_model_data(
			R"({"key":"job","flags":"v","result":{"duration":50,"file":{"fileName":"0:/gcodes/test.gcode","size":100,"printTime":100}}})");
		deliverModelEvents();
	}

	/** The job ends, cancelled or finished, after 1 h 13 min */
	static void endJob(bool cancelled)
	{
		load_model_data(R"({"key":"state","flags":"v","result":{"status":"idle"}})");
		load_model_data(fmt::format(
			R"({{"key":"job","flags":"v","result":{{"lastFileName":"0:/gcodes/test.gcode","lastDuration":4380,"lastFileCancelled":{:s},"lastFileAborted":false,"lastFileSimulated":false}}}})",
			cancelled ? "true" : "false"));
		deliverModelEvents();
	}

	/** Whether the widget called `name` shows: neither it nor anything it is in is hidden */
	bool shows(const char* name) const
	{
		for (lv_obj_t* obj = lv_obj_find_by_name(view.getRootPtr(), name); obj != nullptr; obj = lv_obj_get_parent(obj))
		{
			if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN))
			{
				return false;
			}
		}
		return lv_obj_find_by_name(view.getRootPtr(), name) != nullptr;
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

TEST_F(TestJobWidget, AnEndedJobSaysHowUntilTheUserGoesBackToWhatIsReady)
{
	connectWithFiles("tests/object_model/rr_filelist/gcodes_recent.json",
					 "tests/object_model/rr_filelist/macros_dashboard.json",
					 "0:/gcodes/test.gcode");
	view.show();
	EXPECT_TRUE(shows("job_ready")) << "no job yet: what is ready to start";

	// A job runs, and its status shows; it is cancelled while the screen watches, and its status says so
	runJob();
	EXPECT_TRUE(shows("dashboard_job_status"));
	endJob(true);
	view.updateLayout();
	lv_obj_t* status = lv_obj_find_by_name(view.getRootPtr(), "dashboard_job_status");
	EXPECT_TRUE(shows("dashboard_job_status")) << "it stays until the user goes back";
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(status, "state")), "Cancelled");
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(status, "cancel"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(status, "print_again"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_EQUAL_SCREENSHOT("job_widget/ended.png");

	// Back: what is ready to start
	lv_obj_send_event(
		lv_obj_find_by_name(lv_obj_find_by_name(view.getRootPtr(), "pages"), "back"), LV_EVENT_CLICKED, nullptr);
	view.updateLayout();
	EXPECT_TRUE(shows("job_ready"));
}

TEST_F(TestJobWidget, ReadyShowsTheLastJobTheNewestJobsAndTheMacros)
{
	connectWithFiles("tests/object_model/rr_filelist/gcodes_recent.json",
					 "tests/object_model/rr_filelist/macros_dashboard.json",
					 "0:/gcodes/test.gcode");
	// A job ran, and finished while the screen watched; the user went back from its status
	runJob();
	endJob(false);
	lv_obj_send_event(
		lv_obj_find_by_name(lv_obj_find_by_name(view.getRootPtr(), "pages"), "back"), LV_EVENT_CLICKED, nullptr);
	view.show();
	view.updateLayout();
	ASSERT_TRUE(shows("job_ready"));
	EXPECT_EQUAL_SCREENSHOT("job_widget/ready.png");

	lv_obj_t* root = view.getRootPtr();
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(lv_obj_find_by_name(root, "last"), "detail")),
				 "Finished at 12:00 · took 1 h 13 min");

	// Every job, newest first, without the folder: those on screen, the rest scrolled to
	lv_obj_t* recent = listIn("recent");
	const auto rows = rowsOf(recent);
	ASSERT_GE(rows.size(), 3u);
	EXPECT_EQ(nameOf(rows[0]), "test.gcode");
	EXPECT_EQ(nameOf(rows[2]), "Spool holder.gcode");
	EXPECT_GT(lv_obj_get_scroll_bottom(recent), 0) << "six jobs: more than fit";

	// The first four macros (the widget's default) in the folder's order, named without their ordering prefix or ".g"
	lv_obj_t* macros = listIn("macros");
	ASSERT_EQ(lv_obj_get_child_count(macros), 4u);
	EXPECT_STREQ(lv_label_get_text(lv_obj_find_by_name(lv_obj_get_child(macros, 0), "label")), "Preheat PLA");

	// Tapping a job asks whether to start it, in the shared dialog
	lv_obj_send_event(rows[1], LV_EVENT_CLICKED, nullptr);
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

TEST_F(TestJobWidget, MacrosAreLimitedAsTheWidgetSays)
{
	UI::Xml::XmlView few{"few", screen.getRootPtr(), "dashboard_job", nullptr, {{"macros", "1"}}};
	connectWithFiles("tests/object_model/rr_filelist/gcodes_recent.json",
					 "tests/object_model/rr_filelist/macros_dashboard.json");
	few.show();
	lv_obj_t* root = few.getRootPtr();
	EXPECT_EQ(lv_obj_get_child_count(lv_obj_find_by_name(lv_obj_find_by_name(root, "macros"), "list")), 1u);
}

TEST_F(TestJobWidget, NewFilesAreListedAsThePrinterCountsThem)
{
	connectWithFiles("tests/object_model/rr_filelist/gcodes.json", "tests/object_model/rr_filelist/macros.json");
	view.show();
	ASSERT_EQ(rowsOf(listIn("recent")).size(), 1u);

	// A job uploaded: the printer counts a change to the volume's files, and the jobs are listed again
	load_model_data(R"({"key":"","flags":"d99f","result":{"seqs":{"volChanges":[27,0]}}})");
	deliverModelEvents();
	load_model_data_from_file("tests/object_model/rr_filelist/gcodes_recent.json");
	const auto rows = rowsOf(listIn("recent"));
	ASSERT_GT(rows.size(), 1u);
	EXPECT_EQ(nameOf(rows[1]), "Cable chain link x12.gcode");
}

TEST_F(TestJobWidget, ARunningJobShowsItsStatusAndCanBeFoundAgain)
{
	runJob();
	view.show();
	EXPECT_TRUE(shows("dashboard_job_status"));
	EXPECT_FALSE(shows("job_ready"));

	// Back to what is ready, where the running job leads back to its status
	lv_obj_t* root = view.getRootPtr();
	lv_obj_send_event(lv_obj_find_by_name(lv_obj_find_by_name(root, "pages"), "back"), LV_EVENT_CLICKED, nullptr);
	ASSERT_TRUE(shows("job_ready"));
	EXPECT_FALSE(lv_obj_has_flag(lv_obj_find_by_name(root, "now"), LV_OBJ_FLAG_HIDDEN));
	EXPECT_TRUE(lv_obj_has_flag(lv_obj_find_by_name(root, "last"), LV_OBJ_FLAG_HIDDEN)) << "it runs: nothing to repeat";
	lv_obj_send_event(lv_obj_find_by_name(root, "current"), LV_EVENT_CLICKED, nullptr);
	EXPECT_TRUE(shows("dashboard_job_status"));
}

TEST_F(TestJobWidget, WhileAJobRunsTheDialogSaysWhyNoneCanStartAndItCantBeDeleted)
{
	connectWithFiles("tests/object_model/rr_filelist/gcodes_recent.json",
					 "tests/object_model/rr_filelist/macros_dashboard.json",
					 "0:/gcodes/test.gcode");
	runJob(); // test.gcode, the newest
	lv_obj_t* root = view.getRootPtr();
	view.show();
	lv_obj_send_event(lv_obj_find_by_name(lv_obj_find_by_name(root, "pages"), "back"), LV_EVENT_CLICKED, nullptr);
	const auto rows = rowsOf(listIn("recent"));
	ASSERT_GE(rows.size(), 2u);
	const auto disabled = [](const char* name) { return lv_obj_has_state(TestDialogs::find(name), LV_STATE_DISABLED); };

	// Another job: it can't start until this one has ended
	lv_obj_send_event(rows[1], LV_EVENT_CLICKED, nullptr);
	ASSERT_NE(TestDialogs::shownNamed("file_dialog"), nullptr);
	EXPECT_TRUE(disabled("ok"));
	EXPECT_FALSE(disabled("delete"));
	EXPECT_EQ(TestDialogs::text("reason"), "Another job is running: this one can start once it has ended");
	home.show(); // Whose dialog surface shows the dialog
	lv_obj_update_layout(lv_screen_active());
	EXPECT_EQUAL_SCREENSHOT("job_widget/start_while_running.png");
	home.hide();
	UI::closeDialogs();

	// The job running: it can't be started again, nor deleted
	lv_obj_send_event(rows[0], LV_EVENT_CLICKED, nullptr);
	ASSERT_NE(TestDialogs::shownNamed("file_dialog"), nullptr);
	EXPECT_TRUE(disabled("ok"));
	EXPECT_TRUE(disabled("delete"));
	EXPECT_EQ(TestDialogs::text("reason"),
			  "This is the job running: it can't be started again or deleted until it has ended");

	// It ends with the dialog open: now it can
	endJob(false);
	EXPECT_FALSE(disabled("ok"));
	EXPECT_FALSE(disabled("delete"));
	EXPECT_TRUE(lv_obj_has_flag(TestDialogs::find("reason"), LV_OBJ_FLAG_HIDDEN));
	UI::closeDialogs();
}

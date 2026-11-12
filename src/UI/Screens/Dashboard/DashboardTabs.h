/*
 * DashboardTabs.h
 *
 * The dashboard's tabs: jobs, macros and the print status. While a job is printing the Jobs tab is disabled, and
 * when a job starts the Status tab is shown.
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Components/Containers/TabView.h"
#include "UI/Core/Presenter.h"
#include "UI/Core/View.h"
#include "UI/Screens/File/FileView.h"
#include "UI/Screens/Status/StatusView.h"

namespace UI
{
	class DashboardTabs;

	/** Follows the printer's state and job */
	class DashboardTabsPresenter : public Presenter<DashboardTabs>
	{
	  public:
		PRESENTER_CONSTRUCTOR(DashboardTabsPresenter, DashboardTabs)

		/** Disable the Jobs tab while printing */
		void newStatus(const OM::PrinterStatus& status);

		/** Show the Status tab when a job starts */
		void newJobFileName(const std::string& filename);

	  protected:
		/** Listen for state and job changes */
		void onInit() override;
	};

	class DashboardTabs : public View<DashboardTabsPresenter, TabView>
	{
	  public:
		/** Create the tabs inside `parent` */
		DashboardTabs(const std::string& name, LvObj& parent);

		/** Show the Jobs tab */
		void showJobsTab() { setActiveTab(JOBS_TAB); }

		/** Show the Status tab */
		void showStatusTab() { setActiveTab(STATUS_TAB); }

		/** Enable or disable the Jobs tab */
		void disableJobsTab(bool disable) { disableTab(JOBS_TAB, disable); }

		/** The Jobs file list */
		FileView& getFileView() { return m_fileView; }

		/** The Status tab */
		StatusView& getStatusView() { return m_statusView; }

	  protected:
		/** Give the status tab the shared number pad */
		void onInit() override;

	  private:
		static constexpr size_t JOBS_TAB = 0;	// Index of the Jobs tab
		static constexpr size_t STATUS_TAB = 2; // Index of the Status tab

		FileView m_fileView{
			"files",
			addTab(_("file.jobs")),
			FileView::StorageKeys{.sortBy = {"ui:dashboard:file:jobs:sort_by", OM::FileSystem::SortBy::DATE},
								  .sortDescending = {"ui:dashboard:file:jobs:sort_descending", true},
								  .displayMode = {"ui:dashboard:file:jobs:display_mode", FileView::DisplayMode::List}}};
		FileView m_macroView{
			"macros",
			addTab(_("file.macros")),
			FileView::StorageKeys{
				.sortBy = {"ui:dashboard:file:macros:sort_by", OM::FileSystem::SortBy::NAME},
				.sortDescending = {"ui:dashboard:file:macros:sort_descending", false},
				.displayMode = {"ui:dashboard:file:macros:display_mode", FileView::DisplayMode::List}}};
		StatusView m_statusView{"status", addTab(_("app_drawer.status"))}; // Print status
	};
} // namespace UI

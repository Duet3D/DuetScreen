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
#include "UI/Xml/View/XmlView.h"
#include "i18n/i18n.h"

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

	  private:
		static constexpr size_t JOBS_TAB = 0;	// Index of the Jobs tab
		static constexpr size_t STATUS_TAB = 2; // Index of the Status tab

		// File browsers (file_browser.xml) with the dashboard's own settings (see Screens/XmlScreens.cpp)
		Xml::XmlView m_jobsView{
			"dashboard_jobs", addTab(_("file.jobs")), "file_browser", Xml::Presenters::create("dashboard_jobs")};
		Xml::XmlView m_macrosView{
			"dashboard_macros", addTab(_("file.macros")), "file_browser", Xml::Presenters::create("dashboard_macros")};
		Xml::XmlView m_statusView{"status", addTab(_("app_drawer.status")), "job_status"}; // Print status (job_status.xml)
	};
} // namespace UI

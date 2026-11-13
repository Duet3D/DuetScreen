/*
 * DashboardTabs.cpp
 */

#include "DashboardTabs.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	void DashboardTabsPresenter::onInit()
	{
		registerEventListener<EventType::Status>(this, &DashboardTabsPresenter::newStatus);
		registerEventListener<EventType::JobFileName>(this, &DashboardTabsPresenter::newJobFileName);
	}

	void DashboardTabsPresenter::newStatus(const OM::PrinterStatus& status)
	{
		ZoneScoped;
		getView()->disableJobsTab(OM::IsPrintingStatus(status));
	}

	void DashboardTabsPresenter::newJobFileName(const std::string& filename)
	{
		ZoneScoped;
		if (!filename.empty())
		{
			getView()->showStatusTab();
		}
	}

	DashboardTabs::DashboardTabs(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		setActiveTab(JOBS_TAB);
	}

} // namespace UI

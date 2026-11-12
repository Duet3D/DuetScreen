/*
 * FilesScreen.cpp
 */

#include "FilesScreen.h"
#include "ObjectModel/PrinterStatus.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	void FilesScreenPresenter::onInit()
	{
		registerEventListener<EventType::Status>(this, &FilesScreenPresenter::newStatus);
	}

	void FilesScreenPresenter::newStatus(OM::PrinterStatus status)
	{
		ZoneScoped;
		getView()->disableTab(FilesScreen::JOBS_TAB, OM::IsPrintingStatus(status));
	}

	FilesScreen::FilesScreen(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		setSize(LV_PCT(100), LV_PCT(100));
		addStyle(Themes::getLvglStyles().bg_dark);
		m_macroView.addStyle(Themes::getLvglStyles().card);
		m_jobView.addStyle(Themes::getLvglStyles().card);
		m_macroView.getPresenter()->setBaseFolder(FilePresenter::BaseFolder::MACROS);
		m_jobView.getPresenter()->setBaseFolder(FilePresenter::BaseFolder::GCODES);
	}
} // namespace UI

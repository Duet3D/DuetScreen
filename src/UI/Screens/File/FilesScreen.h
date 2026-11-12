/*
 * FilesScreen.h
 *
 * The Files screen: macros and jobs in tabs. While a job is printing the Jobs tab is disabled, so the print can't be
 * changed under it.
 */

#pragma once

#include "FileView.h"
#include "UI/Components/Containers/TabView.h"
#include "UI/Core/Presenter.h"
#include "UI/Core/View.h"

namespace UI
{
	class FilesScreen;

	/** Disables the Jobs tab while printing */
	class FilesScreenPresenter : public Presenter<FilesScreen>
	{
	  public:
		PRESENTER_CONSTRUCTOR(FilesScreenPresenter, FilesScreen)

		/** Enable or disable the Jobs tab for `status` */
		void newStatus(OM::PrinterStatus status);

	  protected:
		/** Listen for printer status changes */
		void onInit() override;
	};

	class FilesScreen : public View<FilesScreenPresenter, TabView>
	{
	  public:
		/** Create the screen inside `parent` */
		FilesScreen(const std::string& name, LvObj& parent);

		/** Index of the Jobs tab */
		static constexpr size_t JOBS_TAB = 1;

	  private:
		FileView m_macroView{
			"macros",
			addTab(_("file.macros")),
			FileView::StorageKeys{.sortBy = {"ui:home:file:macros:sort_by", OM::FileSystem::SortBy::NAME},
								  .sortDescending = {"ui:home:file:macros:sort_descending", false},
								  .displayMode = {"ui:home:file:macros:display_mode", FileView::DisplayMode::List}}};
		FileView m_jobView{
			"jobs",
			addTab(_("file.jobs")),
			FileView::StorageKeys{.sortBy = {"ui:home:file:jobs:sort_by", OM::FileSystem::SortBy::DATE},
								  .sortDescending = {"ui:home:file:jobs:sort_descending", true},
								  .displayMode = {"ui:home:file:jobs:display_mode", FileView::DisplayMode::Tile}}};
	};
} // namespace UI

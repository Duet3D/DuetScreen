/*
 * Dashboard.h
 *
 *  Created on: 2025-10-02
 *      Author: Andy Everitt
 */

#pragma once

#include "DashboardPresenter.h"
#include "UI/Components/Containers/TabView.h"
#include "UI/Core/View.h"
#include "UI/Screens/File/FileView.h"
#include "UI/Screens/Status/StatusView.h"
#include "UI/Widgets/Temperature/TemperatureGraph.h"
#include "UI/Widgets/ToolList/ToolList.h"

namespace UI
{
	class Dashboard : public View<DashboardPresenter>
	{
	  public:
		Dashboard(const std::string& name, LvObj& parent);

		ToolList& getToolList() { return m_toolList; }
		TemperatureGraph& getGraph() { return m_graph; }
		TabView& getTabs() { return m_tabs; }
		FileView& getFileView() { return m_fileView; }
		StatusView& getStatusView() { return m_statusView; }

		void showJobsTab() { m_tabs.setActiveTab(0); }
		void showStatusTab() { m_tabs.setActiveTab(2); }
		void disableJobsTab(bool disable);

		void setNumberPad(ModalNumberPad* np);

		void clear();

	  protected:
		void onHide() override;

	  private:
		ToolList m_toolList{"tool_list", getRoot(), this};
		TemperatureGraph m_graph{"graph", getRoot()};

		TabView m_tabs{"tabs", getRoot()};
		FileView m_fileView{
			"files",
			m_tabs.addTab(_("file.jobs")),
			FileView::StorageKeys{.sortBy = {"ui:dashboard:file:jobs:sort_by", OM::FileSystem::SortBy::DATE},
								  .sortDescending = {"ui:dashboard:file:jobs:sort_descending", true},
								  .displayMode = {"ui:dashboard:file:jobs:display_mode", FileView::DisplayMode::List}}};
		FileView m_macroView{
			"macros",
			m_tabs.addTab(_("file.macros")),
			FileView::StorageKeys{
				.sortBy = {"ui:dashboard:file:macros:sort_by", OM::FileSystem::SortBy::NAME},
				.sortDescending = {"ui:dashboard:file:macros:sort_descending", false},
				.displayMode = {"ui:dashboard:file:macros:display_mode", FileView::DisplayMode::List}}};
		StatusView m_statusView{"status", m_tabs.addTab(_("app_drawer.status"))};
	};
} // namespace UI

#pragma once

#include "HomePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Graph/Graph.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/MessageBox/AlertMessageBox.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Screen/Screen.h"
#include "UI/Core/View.h"
#include "UI/Screens/Console/ConsoleView.h"
#include "UI/Screens/Fan/FanView.h"
#include "UI/Screens/File/FileView.h"
#include "UI/Screens/Heightmap/HeightmapView.h"
#include "UI/Screens/Move/MoveView.h"
#include "UI/Screens/Settings/SettingsView.h"
#include "UI/Screens/Status/FineTune.h"
#include "UI/Screens/Status/StatusView.h"
#include "UI/Screens/Temperature/TemperatureView.h"
#include "UI/Widgets/SideBar/SideBar.h"
#include "UI/Widgets/StatusBar/StatusBar.h"
#include "UI/Widgets/ToolList/ToolList.h"
#include "lvgl/lvgl.h"
#include <list>
#include <memory>

namespace UI
{
	class HomeView : public View<HomePresenter, Screen>
	{
	  public:
		friend class HomePresenter;
		friend class HomeViewTest;

		static HomeView& instance();
		static void setInstance(HomeView* instance);

		void clear();

		std::shared_ptr<MessageBox> createMessageBox();
		size_t getMessageBoxCount() const { return m_messageBoxList.size(); }
		std::shared_ptr<MessageBox> getMessageBox(size_t index) const;
		void popMessageBox();
		void clearMessageBoxes() { m_messageBoxList.clear(); }

		// Keyboard
		void showKeyboard(bool show);
		LvKeyboard& getKeyboard() { return m_kb; }

		// Sub-views
		StatusBar& getStatusBar() { return m_statusBar; }
		SideBar& getSideBar() { return m_sideBar; }
		ToolList& getToolList() { return m_toolList; }
		Graph& getGraph() { return m_graph; }
		LvContainer& getMainWindow() { return m_mainWindow; }
		ConsoleView& getConsoleView() { return m_consoleView; }
		MoveView& getMoveView() { return m_moveView; }
		TemperatureView& getTemperatureView() { return m_temperatureView; }
		FanView& getFanView() { return m_fanView; }
		FileView& getFileView() { return m_fileView; }
		FileView& getMacroView() { return m_macroView; }
		FineTune& getFineTuneView() { return m_fineTuneView; }
		HeightmapView& getHeightmapView() { return m_heightmapView; }
		SettingsView& getSettingsView() { return m_settingsView; }
		StatusView& getStatusView() { return m_statusView; }

		void showUpdatePrompt(bool show);

		HomeView();
		~HomeView();

	  protected:
	  private:
		// Test seam: when set (by friend HomeViewTest), instance() returns this instead of the static singleton
		static HomeView* s_overrideInstance;
		virtual void onShow();
		virtual void onHide();

		StatusBar m_statusBar;
		SideBar m_sideBar;
		LvContainer m_mainWindow;
		ToolList m_toolList;
		Graph m_graph;

		// Windows
		ConsoleView m_consoleView;
		MoveView m_moveView;
		TemperatureView m_temperatureView;
		FanView m_fanView;
		FileView m_fileView;
		FileView m_macroView;
		FineTune m_fineTuneView;
		HeightmapView m_heightmapView;
		SettingsView m_settingsView;
		StatusView m_statusView;

		// Message box
		std::list<std::shared_ptr<MessageBox>> m_messageBoxList;
		AlertMessageBox m_alert;

		// Update prompt
		MessageBox m_updatePrompt;

		ModalNumberPad m_numberpad;

		LvKeyboard m_kb;
	};
} // namespace UI
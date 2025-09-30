#include "HomeView.h"

#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Settings/SettingsView.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include "utils/utils.h"

namespace UI
{
	HomeView* HomeView::s_overrideInstance = nullptr;

	static constexpr int32_t s_layoutColDsc[3] = {LV_GRID_FR(1), LV_GRID_FR(9), LV_GRID_TEMPLATE_LAST};
	static constexpr int32_t s_layoutRowDsc[3] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

	static constexpr int32_t s_mainWindowLayoutColDsc[3] = {LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
	static constexpr int32_t s_mainWindowLayoutRowDsc[3] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

	HomeView::HomeView()
		: View("HomeView")
		, m_statusBar(getRoot())
		, m_sideBar("sidebar", getRoot())
		, m_mainWindow("main_window", getRoot())
		, m_toolList("tool_list", m_mainWindow, &m_mainWindow)
		, m_graph("graph", m_mainWindow)
		, m_consoleView(m_mainWindow)
		, m_moveView(m_mainWindow)
		, m_temperatureView(m_mainWindow)
		, m_fanView(m_mainWindow)
		, m_fileView(m_mainWindow, &m_mainWindow)
		, m_macroView(m_mainWindow)
		, m_fineTuneView(m_mainWindow)
		, m_heightmapView(m_mainWindow)
		, m_settingsView(m_mainWindow)
		, m_statusView(m_mainWindow)
		, m_alert("alert", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT))
		, m_updatePrompt("update_prompt", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT))
		, m_numberpad("numberpad", m_mainWindow, layout_t(0, 0, LV_SIZE_CONTENT, 100))
		, m_kb("keyboard", m_mainWindow)
	{
		UI_LOCK();
		LOG_INFO("Creating UI");

		lv_obj_set_style_pad_all(lv_screen_active(), 0, LV_PART_MAIN);

		addStyle(Themes::getLvglStyles().bg_dark);
		addStyle(Themes::getLvglStyles().pad_zero);
		m_toolList.addStyle(Themes::getLvglStyles().card);
		m_graph.addStyle(Themes::getLvglStyles().card);
		m_macroView.addStyle(Themes::getLvglStyles().card);

		addHomeScreen(this);

		setLayoutStyle(LV_LAYOUT_GRID);
		setGridDsc(s_layoutColDsc, s_layoutRowDsc);
		setGridCell(m_statusBar, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_sideBar, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
		setGridCell(m_mainWindow, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

		m_statusBar.setHeight(LV_SIZE_CONTENT);
		m_mainWindow.setFlexGrow(1);
		m_mainWindow.setHeight(LV_PCT(100));
		m_fileView.addStyle(Themes::getLvglStyles().card);

		// Main Window Layout
		m_mainWindow.setLayoutStyle(LV_LAYOUT_GRID);
		m_mainWindow.setGridDsc(s_mainWindowLayoutColDsc, s_mainWindowLayoutRowDsc);
		m_mainWindow.setGridCell(m_toolList, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		m_mainWindow.setGridCell(m_graph, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		m_mainWindow.setGridCell(m_fileView, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
		m_mainWindow.setGridCell(m_statusView, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
		m_mainWindow.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		// Graph
		m_graph.setXRange({.min = -60, .max = 0});
		m_graph.setYRange({.min = 0, .max = 300});
		m_graph.setXCount(-m_graph.getXRange().min * MODEL_TICK_HZ * 2);

		m_consoleView.hide();
		m_moveView.hide();
		m_temperatureView.hide();
		m_fanView.hide();
		m_macroView.hide();
		m_fineTuneView.hide();
		m_heightmapView.hide();
		m_statusView.hide();
		m_settingsView.hide();

		m_macroView.getPresenter()->setBaseFolder(FilePresenter::BaseFolder::MACROS);

		// Message Box
		m_alert.hide();
		m_alert.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_alert.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_alert.setMaxHeight(LV_PCT(70));

		// Update Prompt
		m_updatePrompt.hide();
		m_updatePrompt.setTitle(_("update_available"));
		m_updatePrompt.setText(_("update_available_text"));
		m_updatePrompt.setOkBtnText(_("update_confirm"));
		m_updatePrompt.setCancelBtnText(_("update_cancel"));
		m_updatePrompt.setOkCallback([this]() { m_presenter->update(); });
		m_updatePrompt.okVisible(true);
		m_updatePrompt.cancelVisible(true);
		m_updatePrompt.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_updatePrompt.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_updatePrompt.setMaxHeight(LV_PCT(70));

		// Keyboard
		showKeyboard(false);
		m_kb.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_kb.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
		m_kb.setSize(LV_PCT(100), LV_PCT(50));

		m_numberpad.hide();
	}

	HomeView::~HomeView()
	{
		removeHomeScreen(this, false);
	}

	HomeView& HomeView::instance()
	{
		// Allow tests to override the singleton instance when needed
		if (s_overrideInstance)
		{
			return *s_overrideInstance;
		}
		static HomeView view;
		return view;
	}

	void HomeView::setInstance(HomeView* instance)
	{
		s_overrideInstance = instance;
	}

	void HomeView::clear()
	{
		m_graph.clear();
		m_toolList.setItemCnt(0);
		clearMessageBoxes();
		m_toolList.hideNumberPad();
	}

	void HomeView::onShow()
	{
		m_fileView.show();
		m_toolList.activate();
		m_statusBar.activate();
		m_sideBar.show(true);
	}

	void HomeView::onHide()
	{
		// Clear the tool list
		m_graph.clear();
		m_toolList.deactivate();
		m_toolList.setItemCnt(0);
		m_toolList.hideNumberPad();
	}

	std::shared_ptr<MessageBox> HomeView::createMessageBox()
	{
		UI_LOCK();
		m_messageBoxList.emplace_back(
			std::make_shared<MessageBox>("home_message_box", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)));
		std::shared_ptr<MessageBox> msgBox = m_messageBoxList.back();
		msgBox->setFlag(LV_OBJ_FLAG_FLOATING, true);
		msgBox->setAlign(LV_ALIGN_TOP_MID, 0, 2);
		msgBox->setMaxHeight(LV_PCT(70));
		msgBox->hide();
		return msgBox;
	}

	std::shared_ptr<MessageBox> HomeView::getMessageBox(size_t index) const
	{
		UI_LOCK();
		if (index >= getMessageBoxCount())
		{
			return nullptr;
		}
		auto it = m_messageBoxList.cbegin();
		std::advance(it, index);
		return (*it);
	}

	void HomeView::popMessageBox()
	{
		UI_LOCK();
		if (!m_messageBoxList.empty())
		{
			m_messageBoxList.erase(m_messageBoxList.begin());
		}
	}

	void HomeView::showKeyboard(bool show)
	{
		UI_LOCK();
		m_alert.setAlign(LV_ALIGN_TOP_MID, 0, show ? 5 : 0);
		m_alert.setMaxHeight(show ? LV_PCT(45) : LV_PCT(70));
		m_kb.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
	}

	void HomeView::showUpdatePrompt(bool show)
	{
		UI_LOCK();
		m_updatePrompt.setVisible(show, true);
	}
} // namespace UI

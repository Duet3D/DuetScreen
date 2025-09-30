/*
 * ToolList.cpp
 *
 *  Created on: 2025-01-16
 *      Author: Andy Everitt
 */

#include "ToolList.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	ToolListItem::ToolListItem(size_t index, LvObj& parent, ToolList& toolList)
		: View(fmt::format("{:d}", index), parent)
		, m_toolList(toolList)
		, m_toolName("name", getRoot())
		, m_status("status", getRoot())
		, m_currentTemp("current_temp", getRoot())
		, m_activeTemp("active_temp", getRoot())
		, m_standbyTemp("standby_temp", getRoot())
	{
		setSize(LV_PCT(100), LV_SIZE_CONTENT);

		UI_LOCK();
		setStylePad(5, LV_PART_MAIN, Padding::ALL);
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_toolName.setFlexGrow(4);
		m_status.setFlexGrow(3);
		m_currentTemp.setFlexGrow(2);
		m_activeTemp.setFlexGrow(2);
		m_standbyTemp.setFlexGrow(2);

		m_activeTemp.setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		m_standbyTemp.setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		m_activeTemp.setExtClickArea(20);
		m_standbyTemp.setExtClickArea(20);

		m_toolName.addClickedCallback(onNameEvent, this);
		m_status.addClickedCallback(onStatusEvent, this);
		m_activeTemp.addEventCallback(onActiveStandbyEvent, LV_EVENT_CLICKED, this);
		m_standbyTemp.addEventCallback(onActiveStandbyEvent, LV_EVENT_CLICKED, this);

		// Styles
		addStyle(Themes::getLvglStyles().bg_color_list_item);
		// addStyle(Themes::getLvglStyles().border_color_secondary, LV_STATE_CHECKED);
		addStyle(Themes::getComponentStyles().tool_selected, LV_STATE_CHECKED);

		m_toolName.addStyle(Themes::getLvglStyles().actionBtn);
		m_toolName.addStyle(Themes::getLvglStyles().border_color_card, LV_STATE_CHECKED);
		m_status.addStyle(Themes::getLvglStyles().border_color_card, LV_STATE_CHECKED);
		m_status.addStyle(Themes::getLvglStyles().actionBtn);
		m_activeTemp.addStyle(Themes::getLvglStyles().input);
		m_standbyTemp.addStyle(Themes::getLvglStyles().input);

		m_toolName.getLabel().setAlign(LV_ALIGN_LEFT_MID, 0, 0);
	}

	uint8_t ToolListItem::getSlotIndex() const
	{
		UI_LOCK();
		return (uint8_t)m_presenter->getSlotIndex();
	}

	void ToolListItem::setSlotIndex(uint8_t index)
	{
		UI_LOCK();
		m_presenter->setSlotIndex(index);
	}

	void ToolListItem::setLabel(std::string_view text)
	{
		m_toolName.setText(text);
	}

	void ToolListItem::setIcon(lv_img_dsc_t* icon)
	{
		return;
	}

	void ToolListItem::setSelected(const bool selected)
	{
		if (m_selected == selected)
		{
			return;
		}
		UI_LOCK();
		lv_color_t color = lv_obj_get_style_bg_color(getRoot(), LV_PART_MAIN);
		setState(LV_STATE_CHECKED, selected);
		m_selected = selected;
	}

	void ToolListItem::setStatus(std::string_view text)
	{
		m_status.setText(text);
	}

	void ToolListItem::setCurrentTemp(float value)
	{
		m_currentTemp.setText(fmt::format("{:.1f}", value));
	}

	void ToolListItem::setActiveTemp(int32_t value)
	{
		m_activeTemp.setText(fmt::format("{:d}", value));
	}

	void ToolListItem::setActiveTempText(std::string_view text)
	{
		m_activeTemp.setText(text);
	}

	void ToolListItem::setStandbyTemp(int32_t value)
	{
		m_standbyTemp.setText(fmt::format("{:d}", value));
	}

	void ToolListItem::setStandbyTempText(std::string_view text)
	{
		m_standbyTemp.setText(text);
	}

	void ToolListItem::showTemps(bool show)
	{
		UI_LOCK();
		m_currentTemp.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
		m_activeTemp.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
		m_standbyTemp.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
		m_status.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
	}

	void ToolListItem::onNameEvent(lv_event_t* e)
	{
		UI_LOCK();
		ToolListItem* view = static_cast<ToolListItem*>(lv_event_get_user_data(e));
		view->m_presenter->toggleState();
	}

	void ToolListItem::onStatusEvent(lv_event_t* e)
	{
		UI_LOCK();
		ToolListItem* view = static_cast<ToolListItem*>(lv_event_get_user_data(e));
		view->m_presenter->toggleSubState();
	}

	void ToolListItem::onActiveStandbyEvent(lv_event_t* e)
	{
		UI_LOCK();
		ToolListItem* view = static_cast<ToolListItem*>(lv_event_get_user_data(e));
		lv_obj_t* obj = lv_event_get_target_obj(e);

		if (!view->m_presenter->configureNumberPad(obj == view->m_activeTemp))
		{
			LOG_WARN("Failed to configure number pad");
			view->getToolList().hideNumberPad();
			return;
		}
		view->getToolList().showNumberPad();
	}

	ToolList::ToolList(const std::string& name, LvObj& parent, LvObj* numberPadParent)
		: View(name, parent)
		, m_header("header", getRoot())
		, m_headerTool("tool", m_header)
		, m_headerStatus("status", m_header)
		, m_headerCurrent("current", m_header)
		, m_headerActive("active", m_header)
		, m_headerStandby("standby", m_header)
		, m_list("list", getRoot())
		, m_numberPad("number_pad", numberPadParent ? *numberPadParent : parent, layout_t(0, 0, 50, 70))
	{
		init();
	}

	void ToolList::init()
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_header.addStyle(Themes::getLvglStyles().bg_color_header);

		m_header.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_list.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		addStyle(Themes::getLvglStyles().pad_zero);
		m_header.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);

		m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_list.setWidth(LV_PCT(100));
		m_list.setFlexGrow(1);
		m_list.setListGrow(1);
		m_header.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_headerTool.setFlexGrow(4);
		m_headerStatus.setFlexGrow(3);
		m_headerCurrent.setFlexGrow(2);
		m_headerActive.setFlexGrow(2);
		m_headerStandby.setFlexGrow(2);

		m_headerTool.setText(_("toollist_tool"));
		m_headerStatus.setText(_("toollist_status"));
		m_headerCurrent.setText(_("toollist_current"));
		m_headerActive.setText(_("toollist_active"));
		m_headerStandby.setText(_("toollist_standby"));

		m_headerTool.setStyleTextAlign(LV_TEXT_ALIGN_LEFT, 0);

		// Number Pad

		m_numberPad.hide();
	}

	void ToolList::setItemCnt(size_t cnt)
	{
		m_list.setItemCount(cnt, *this);
	}

	void ToolList::showNumberPad()
	{
		UI_LOCK();
		m_numberPad.clear();
		openModal(&m_numberPad);
		// m_numberPad.show();
	}

} // namespace UI

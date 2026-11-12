/*
 * ToolList.cpp
 *
 *  Created on: 2025-01-16
 *      Author: Andy Everitt
 */

#include "ToolList.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"

#define SHOW_HEADERS 0

namespace UI
{
	ToolList::ToolList(const std::string& name, LvObj& parent, LvObj* numberPadParent)
		: View(name, parent)
		, m_numberPad("number_pad", numberPadParent ? *numberPadParent : parent)
	{
		ZoneScoped;
		init();
	}

	void ToolList::init()
	{
		ZoneScoped;
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		addStyle(Themes::getLvglStyles().pad_zero);

		m_tools.setTitle(_("temperature.tools"));
		m_beds.setTitle(_("temperature.beds"));
		m_chambers.setTitle(_("temperature.chambers"));
		m_tools.getHeader().setVisible(false);
		m_beds.getHeader().setVisible(false);
		m_chambers.getHeader().setVisible(false);

		// m_beds.getListContainer().addStyle(Themes::getComponentStyles().tool_list_tool);
		// m_chambers.getListContainer().addStyle(Themes::getComponentStyles().tool_list_tool);

		m_tools.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_beds.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_chambers.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_tools.setStylePad(0);
		m_beds.setStylePad(0);
		m_chambers.setStylePad(0);

		m_tools.getHeader().setStylePad(0);
		m_beds.getHeader().setStylePad(0);
		m_chambers.getHeader().setStylePad(0);
	}

	size_t ToolList::setToolCount(size_t count)
	{
		ZoneScoped;
#if SHOW_HEADERS
		m_tools.getHeader().setVisible(count > 1);
#endif
		return m_tools.setItemCount(count,
									[this](size_t index, LvObj& parent)
									{
										auto item = std::make_unique<ToolListTool>(index, parent, *this);
										item->activate();
										return item;
									});
	}

	size_t ToolList::setBedCount(size_t count)
	{
		ZoneScoped;
#if SHOW_HEADERS
		m_beds.getHeader().setVisible(count > 1);
#endif
		return m_beds.setItemCount(count);
	}

	size_t ToolList::setChamberCount(size_t count)
	{
		ZoneScoped;
#if SHOW_HEADERS
		m_chambers.getHeader().setVisible(count > 1);
#endif
		return m_chambers.setItemCount(count);
	}

	ToolListHeater* ToolList::getBed(size_t index) const
	{
		ZoneScoped;
		auto bed = m_beds.getItem(index);
		if (!bed)
		{
			return nullptr;
		}
		return &bed->getHeater();
	}

	ToolListHeater* ToolList::getChamber(size_t index) const
	{
		ZoneScoped;
		auto chamber = m_chambers.getItem(index);
		if (!chamber)
		{
			return nullptr;
		}
		return &chamber->getHeater();
	}

	void ToolList::showNumberPad()
	{
		ZoneScoped;
		UI_LOCK();
		m_numberPad.clear();
		openModal(&m_numberPad);
	}

	ToolListTool::ToolListTool(size_t index, LvObj& parent, ToolList& toolList)
		: View(index, parent)
	{
		ZoneScoped;
		getPresenter()->setToolListPresenter(toolList.getPresenter());

		setSize(LV_PCT(100), LV_SIZE_CONTENT);

		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_toolName.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_heaters.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		// m_heaters.setStylePad(0, LV_PART_MAIN, Padding::TOP);
		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::BOTTOM);
		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::RIGHT);
		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::LEFT);
		m_heaters.getListContainer().setStylePad(0);

		m_toolName.addClickedCallback(onNameEvent, this);

		// Styles
		// addStyle(Themes::getLvglStyles().border_color_secondary, LV_STATE_CHECKED);
		addStyle(Themes::getComponentStyles().tool_list_tool);
		addStyle(Themes::getComponentStyles().tool_selected, LV_STATE_CHECKED);

		m_toolName.addStyle(Themes::getLvglStyles().actionBtn);
		m_toolName.addStyle(Themes::getLvglStyles().border_color_card, LV_STATE_CHECKED);

		m_toolName.getLabel().setAlign(LV_ALIGN_LEFT_MID, 0, 0);
	}

	ToolListBedChamber::ToolListBedChamber(size_t index, LvObj& parent)
		: ListItem(index, parent)
	{
		ZoneScoped;
		setSize(LV_PCT(100), LV_SIZE_CONTENT);

		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_heaters.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::TOP);
		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::BOTTOM);
		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::RIGHT);
		m_heaters.setStylePad(0, LV_PART_MAIN, Padding::LEFT);
		m_heaters.getListContainer().setStylePad(0);

		m_heaters.setItemCount(1);

		// Styles
		addStyle(Themes::getComponentStyles().tool_list_tool);
		addStyle(Themes::getComponentStyles().tool_selected, LV_STATE_CHECKED);
	}

	ToolListHeater::ToolListHeater(size_t index, LvObj& parent)
		: ListItem(index, parent)
	{
		ZoneScoped;
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		setStyleTextAlign(LV_TEXT_ALIGN_CENTER);

		setSize(LV_PCT(100), LV_SIZE_CONTENT);

		m_label.setStyleTextAlign(LV_TEXT_ALIGN_LEFT);

		m_label.setFlexGrow(3);
		m_status.setFlexGrow(3);
		m_currentTemp.setFlexGrow(2);
		m_activeTemp.setFlexGrow(2);
		m_standbyTemp.setFlexGrow(2);

		// m_activeTemp.setExtClickArea(20);
		// m_standbyTemp.setExtClickArea(20);

		m_status.addClickedCallback(onStatusEvent, this);
		m_activeTemp.addEventCallback(onActiveStandbyEvent, LV_EVENT_CLICKED, this);
		m_standbyTemp.addEventCallback(onActiveStandbyEvent, LV_EVENT_CLICKED, this);

		addStyle(Themes::getComponentStyles().tool_list_heater);
		m_status.addStyle(Themes::getLvglStyles().border_color_card, LV_STATE_CHECKED);
		m_status.addStyle(Themes::getLvglStyles().actionBtn);
		m_activeTemp.addStyle(Themes::getLvglStyles().input);
		m_standbyTemp.addStyle(Themes::getLvglStyles().input);
	}

	void ToolListTool::setLabel(std::string_view text)
	{
		ZoneScoped;
		m_toolName.setText(text);
	}

	void ToolListTool::setSelected(const bool selected)
	{
		ZoneScoped;
		if (m_selected == selected)
		{
			return;
		}
		UI_LOCK();
		setState(LV_STATE_CHECKED, selected);
		m_selected = selected;
	}

	void ToolListTool::setDisabled(bool disabled)
	{
		ZoneScoped;
		UI_LOCK();
		m_toolName.setDisabled(disabled);

		m_heaters.iterateListItems([disabled](size_t /* index */, ToolListHeater& heater)
								   { heater.setDisabled(disabled); });
	}

	size_t ToolListTool::setHeaterCount(size_t count)
	{
		ZoneScoped;
		m_heaters.setFlag(LV_OBJ_FLAG_HIDDEN, count == 0);
		return m_heaters.setItemCount(count);
	}

	void ToolListTool::onNameEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		auto view = static_cast<ToolListTool*>(lv_event_get_user_data(e));
		view->m_presenter->toggleState();
	}

	ToolListHeater& ToolListBedChamber::getHeater()
	{
		ZoneScoped;
		return *m_heaters.getItem(0);
	}

	void ToolListHeater::setLabel(std::string_view text)
	{
		ZoneScoped;
		m_label.setText(text);
	}

	void ToolListHeater::setStatus(std::string_view status)
	{
		ZoneScoped;
		m_status.setText(status);
	}

	void ToolListHeater::setCurrentTemp(float value)
	{
		ZoneScoped;
		m_currentTemp.setText(fmt::format("{:.1f}", value));
	}

	void ToolListHeater::setActiveTemp(int32_t value)
	{
		ZoneScoped;
		m_activeTemp.setText(fmt::format("{:d}", value));
	}

	void ToolListHeater::setStandbyTemp(int32_t value)
	{
		ZoneScoped;
		m_standbyTemp.setText(fmt::format("{:d}", value));
	}

	void ToolListHeater::setDisabled(bool disabled)
	{
		ZoneScoped;
		UI_LOCK();
		m_status.setDisabled(disabled);
	}

	void ToolListHeater::onStatusEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		auto view = static_cast<ToolListHeater*>(lv_event_get_user_data(e));
		if (view->m_statusCb)
			view->m_statusCb();
	}

	void ToolListHeater::onActiveStandbyEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		auto view = static_cast<ToolListHeater*>(lv_event_get_user_data(e));
		LvObj* obj = LvObj::fromPtr(lv_event_get_target_obj(e));

		if (view->m_temperatureCb)
			view->m_temperatureCb(obj == &view->m_activeTemp);
	}
} // namespace UI

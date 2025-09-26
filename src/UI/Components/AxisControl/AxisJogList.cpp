/*
 * AxisJogList.cpp
 *
 *  Created on: 2025-06-11
 *      Author: Andy Everitt
 */

#include "AxisJogList.h"
#include "Debug.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	AxisItem::AxisItem(const size_t index, LvObj& parent)
		: ListItem(index, parent)
		, m_home("home", getRoot(), "", layout_t(0, 0, 0, 100))
		, m_relMove("rel_move", getRoot())
		, m_toolPosition("tool_position", getRoot())
		, m_machinePosition("machine_position", getRoot())
	{
		// Layout
		UI_LOCK();
		constexpr lv_coord_t pad = 2;
		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setStylePad(pad, LV_PART_MAIN, Padding::ALL);
		setStylePad(pad, LV_PART_MAIN, Padding::COLUMN);
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_home.setSize(LV_SIZE_CONTENT, LV_PCT(100));

		m_relMove.setListFlow(LV_FLEX_FLOW_ROW);
		m_relMove.setStylePad(0);
		m_relMove.setListPad(0);
		m_relMove.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_relMove.setListSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		lv_obj_set_flex_grow(m_toolPosition, 5);
		lv_obj_set_flex_grow(m_machinePosition, 5);
		lv_obj_set_style_text_align(m_toolPosition, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_text_align(m_machinePosition, LV_TEXT_ALIGN_CENTER, 0);

		lv_label_set_text(m_toolPosition, "");
		lv_label_set_text(m_machinePosition, "");

		m_home.addClickedCallback(onHomeEvent, this);
		m_home.addStyle(Themes::getLvglStyles().actionBtn, 0);

		// Styles
		m_home.setCheckable(true);
		m_home.addStyle(Themes::getComponentStyles().unhomed, LV_STATE_CHECKED);
	}

	AxisItem::~AxisItem() {}

	void AxisItem::setAxisLetter(const char* letter)
	{
		UI_LOCK();
		m_axisLetter = letter;
		m_home.setText(utils::format(_("move_axis_home"), letter).c_str());
	}

	void AxisItem::setJogAmounts(const float* distances, const size_t count)
	{
		UI_LOCK();
		m_relMove.clear();

		if (count == 0)
		{
			return;
		}

		if (distances == nullptr)
		{
			LOG_ERROR("distances is null");
			return;
		}

		m_relMove.setItemCount(count,
							   [&distances, this](size_t i, LvObj& parent)
							   {
								   auto btn = std::make_shared<Button>(
									   fmt::format("{}", i), parent, fmt::format("{:.1f}", distances[i]));
								   btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
								   btn->addClickedCallback(onRelMoveEvent, this);
								   btn->setFlexGrow(1);
								   btn->setHeight(LV_SIZE_CONTENT);
								   btn->addStyle(Themes::getLvglStyles().actionBtn, 0);
								   return btn;
							   });
	}

	void AxisItem::setJogAmounts(const std::vector<float>& distances)
	{
		UI_LOCK();
		setJogAmounts(distances.data(), distances.size());
	}

	void AxisItem::setHomed(const bool homed)
	{
		UI_LOCK();
		m_home.setChecked(!homed);
	}

	void AxisItem::setToolPosition(const float& position)
	{
		UI_LOCK();
		lv_label_set_text(m_toolPosition, utils::format("%.2f", position).c_str());
	}

	void AxisItem::setMachinePosition(const float& position)
	{
		UI_LOCK();
		lv_label_set_text(m_machinePosition, utils::format("(%.2f)", position).c_str());
	}

	void AxisItem::disableHome(const bool disabled)
	{
		UI_LOCK();
		m_home.setDisabled(disabled);
		m_home.setText(disabled ? m_axisLetter.c_str()
								: utils::format(_("move_axis_home"), m_axisLetter.c_str()).c_str());
	}

	void AxisItem::setJogCallback(jog_cb_t cb, void* user_data)
	{
		UI_LOCK();
		m_jogCallback = cb;
		m_jogUserData = user_data;
	}

	void AxisItem::setHomeCallback(home_cb_t cb, void* user_data)
	{
		UI_LOCK();
		m_homeCallback = cb;
		m_homeUserData = user_data;
	}

	void AxisItem::onHomeEvent(lv_event_t* e)
	{
		UI_LOCK();
		AxisItem* item = static_cast<AxisItem*>(lv_event_get_user_data(e));
		if (item == nullptr)
		{
			LOG_ERROR("item is null");
			return;
		}
		if (item->m_homeCallback)
		{
			item->m_homeCallback(item->getIndex(), item->m_homeUserData);
		}
	}

	void AxisItem::onRelMoveEvent(lv_event_t* e)
	{
		UI_LOCK();
		AxisItem* item = static_cast<AxisItem*>(lv_event_get_user_data(e));
		lv_obj_t* btn = (lv_obj_t*)lv_event_get_target_obj(e);
		uintptr_t index = (uintptr_t)lv_obj_get_user_data(btn);

		if (item->m_jogCallback)
		{
			item->m_jogCallback(item->getIndex(), index, item->m_jogUserData);
		}
	}

	AxisJogList::AxisJogList(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
		, m_axisItems("axis_jog_list_item", getRoot())
		, m_listHeaderPadding("axis_jog_list_header_padding", m_axisItems.getHeader())
		, m_toolPositionLabel("tool_position_label", m_axisItems.getHeader())
		, m_machinePositionLabel("machine_position_label", m_axisItems.getHeader())
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

		addStyle(Themes::getLvglStyles().no_border, 0);

		m_axisItems.setListGrow(1);
		m_axisItems.setSize(LV_PCT(100), LV_PCT(100));
		m_axisItems.showHeader(true);

		LvContainer& header = m_axisItems.getHeader();
		m_listHeaderPadding.setHeight(0);
		m_listHeaderPadding.addStyle(Themes::getLvglStyles().no_border, 0);

		m_listHeaderPadding.setFlexGrow(20);
		m_toolPositionLabel.setFlexGrow(5);
		m_machinePositionLabel.setFlexGrow(5);

		m_toolPositionLabel.setText(_("move_tool_position"));
		m_machinePositionLabel.setText(_("move_machine_position"));

		m_toolPositionLabel.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_machinePositionLabel.setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
	}
} // namespace UI

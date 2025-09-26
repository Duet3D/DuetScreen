/*
 * AxisJogList.h
 *
 *  Created on: 2025-06-11
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"

namespace UI
{
	class AxisItem : public ListItem
	{
		using jog_cb_t = std::function<void(size_t axis_index, size_t jog_index, void* user_data)>;
		using home_cb_t = std::function<void(size_t axis_index, void* user_data)>;

	  public:
		AxisItem(const size_t index, LvObj& parent);
		virtual ~AxisItem();

		void setAxisLetter(const char* letter);
		void setJogAmounts(const float* distances, const size_t count);
		void setJogAmounts(const std::vector<float>& distances);
		void setHomed(const bool homed);
		void setToolPosition(const float& position);
		void setMachinePosition(const float& position);
		void disableHome(const bool show);

		void setJogCallback(jog_cb_t cb, void* user_data = nullptr);
		void setHomeCallback(home_cb_t cb, void* user_data = nullptr);

	  private:
		static void onHomeEvent(lv_event_t* e);
		static void onRelMoveEvent(lv_event_t* e);

		Button m_home;
		List<Button> m_relMove;
		LvLabel m_toolPosition;
		LvLabel m_machinePosition;

		std::string m_axisLetter;
		jog_cb_t m_jogCallback;
		home_cb_t m_homeCallback;
		void* m_jogUserData = nullptr;
		void* m_homeUserData = nullptr;
	};

	class AxisJogList : public LvContainer
	{
	  public:
		AxisJogList(const std::string& name, LvObj& parent);

		size_t getItemCount() const { return m_axisItems.getItemCount(); }
		void setItemCount(const size_t count) { m_axisItems.setItemCount(count); }
		List<AxisItem>& getAxisItems() { return m_axisItems; }

	  private:
		List<AxisItem> m_axisItems;
		LvContainer m_listHeaderPadding;
		LvLabel m_toolPositionLabel;
		LvLabel m_machinePositionLabel;
	};
} // namespace UI

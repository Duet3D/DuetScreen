/*
 * ToolList.h
 *
 *  Created on: 2025-01-16
 *      Author: Andy Everitt
 */

#pragma once

#include "ToolListPresenter.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/List/List.h"
#include "UI/Core/View.h"
#include <memory>
#include <vector>

namespace UI
{
	class ToolList;

	class ToolListItem : public View<ToolListItemPresenter>
	{
	  public:
		ToolListItem(size_t index, LvObj& parent, ToolList& toolList);

		uint8_t getSlotIndex() const;
		void setSlotIndex(uint8_t index);
		void setLabel(std::string_view text);
		void setIcon(lv_img_dsc_t* icon);
		void setSelected(const bool selected);
		void setStatus(std::string_view text);
		void setCurrentTemp(float value);
		void setActiveTemp(int32_t value);
		void setActiveTempText(std::string_view text);
		void setStandbyTemp(int32_t value);
		void setStandbyTempText(std::string_view text);
		void showTemps(bool show);

		ToolList& getToolList() const { return m_toolList; }

	  private:
		static void onNameEvent(lv_event_t* e);
		static void onStatusEvent(lv_event_t* e);
		static void onActiveStandbyEvent(lv_event_t* e);

		ToolList& m_toolList;

		Button m_toolName;
		Button m_status;
		LvLabel m_currentTemp;
		Button m_activeTemp;
		Button m_standbyTemp;

		bool m_selected;
	};

	class ToolList : public View<ToolListPresenter>
	{
	  public:
		friend class ToolListPresenter;
		friend class ToolListItemPresenter;

		ToolList(const std::string& name, LvObj& parent, LvObj* numberPadParent = nullptr);

		void setItemCnt(size_t cnt);
		size_t getItemCnt() const { return m_list.getItemCount(); }
		std::shared_ptr<ToolListItem> getToolListItem(size_t index) const { return m_list.getItem(index); }

		void showNumberPad();
		void hideNumberPad() { m_numberPad.hide(); }

		virtual void onHide() override { hideNumberPad(); }

	  private:
		void init();

		LvContainer m_header;
		LvLabel m_headerTool;
		LvLabel m_headerStatus;
		LvLabel m_headerCurrent;
		LvLabel m_headerActive;
		LvLabel m_headerStandby;
		List<ToolListItem> m_list;

		ModalNumberPad m_numberPad;
	};
} // namespace UI

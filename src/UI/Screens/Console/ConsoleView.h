#pragma once

#include "ConsolePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/LVGL/LvTextArea.h"
#include "UI/Core/View.h"

namespace UI
{
	class ConsoleView : public View<ConsolePresenter>
	{
	  public:
		friend class ConsoleItem;
		friend class ConsolePresenter;

		ConsoleView(LvObj& parent);

		void clear();
		void addCommand(const char* resp);
		void addResponse(const char* resp);
		void showCommandList(bool show, bool animate = LV_ANIM_ON);
		void showKeyboard(bool show);

		bool back() override;

	  private:
		static void onSendEvent(lv_event_t* e);
		static void onClearEvent(lv_event_t* e);
		static void onCommandListEvent(lv_event_t* e);
		static void onKeyboardEvent(lv_event_t* e);

		void updateBtnPos();
		
		virtual void onShow() override;
		virtual void onHide() override;

		LvContainer m_topCont{"top_cont", getRoot()};
		LvObj m_commandList{lv_table_create, "command_list", m_topCont};
		Button m_commandVisibility{"command_visibility", m_topCont, LV_SYMBOL_LIST};
		LvTextArea m_output{"output", m_topCont};
		LvContainer m_inputCont{"input_cont", getRoot()};
		LvTextArea m_input{"input", m_inputCont};
		Button m_clear{"clear", m_input, LV_SYMBOL_TRASH};
		Button m_enter{"enter", m_inputCont, LV_SYMBOL_NEW_LINE};
		LvKeyboard m_kb{"keyboard", getRoot()};
	};
} // namespace UI
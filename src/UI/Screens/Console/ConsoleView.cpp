#include "ConsoleView.h"
#include "Debug.h"
#include "Gcodes.h"
#include "Hardware/Duet.h"
#include "UI/Components/LVGL/LvAnim.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include "utils/StorageHelper.h"

namespace UI
{
#define TABLE_GCODE_WIDTH 100
#define TABLE_DESCRIPTION_WIDTH 500

	ConsoleView::ConsoleView(LvObj& parent)
		: View("console_view", parent, layout_t(0, 0, 100, 100))
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);

		// Layout
		setAlign(LV_ALIGN_CENTER, 0, 0);
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_topCont.setWidth(LV_PCT(100));
		m_topCont.setFlexGrow(1);
		m_inputCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		// Top Container
		m_topCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_commandVisibility.setCheckable(false);
		m_commandVisibility.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		m_commandVisibility.updateLayout();
		m_commandList.setFlexGrow(1);
		m_commandList.setMinWidth(TABLE_GCODE_WIDTH);
		m_output.setFlexGrow(30);
		m_commandList.setHeight(LV_PCT(100));
		m_output.setHeight(LV_PCT(100));
		m_output.setCursorClickPos(false);

		// Command List
		lv_table_set_column_count(m_commandList, 2);
		lv_table_set_column_width(m_commandList, 0, TABLE_GCODE_WIDTH);
		lv_table_set_column_width(m_commandList, 1, TABLE_DESCRIPTION_WIDTH);
		lv_table_set_row_count(m_commandList, Gcodes::getGcodeCount());
		for (size_t i = 0; i < Gcodes::getGcodeCount(); i++)
		{
			const gcode* g = Gcodes::getGcode(i);
			lv_table_set_cell_value(m_commandList, i, 0, g->gcode);
			lv_table_set_cell_value(m_commandList, i, 1, g->helpText);
		}

		// Input Area
		m_inputCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_inputCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_inputCont.addStyle(Themes::getLvglStyles().pad_zero);

		m_input.setFlexGrow(1);
		m_input.setOneLine(true);
		m_input.setPlaceholderText(_("console_input_placeholder"));
		m_input.setStyleTextAlign(LV_TEXT_ALIGN_LEFT, 0);
		m_clear.setAlign(LV_ALIGN_RIGHT_MID, 0, 0);
		m_input.setHeight(LV_SIZE_CONTENT);
		m_clear.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_enter.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		m_topCont.addStyle(Themes::getLvglStyles().no_border);
		m_inputCont.addStyle(Themes::getLvglStyles().no_border);
		m_input.addStyle(Themes::getLvglStyles().pad_zero);
		m_clear.addStyle(Themes::getLvglStyles().pad_zero);
		m_enter.addStyle(Themes::getLvglStyles().pad_zero);

		// Hide keyboard initially
		m_kb.setSize(LV_PCT(100), LV_PCT(40));
		m_kb.setMode(LV_KEYBOARD_MODE_TEXT_UPPER);
		m_kb.hide();

		// Callbacks
		m_clear.addClickedCallback(onClearEvent, this);
		m_enter.addClickedCallback(onSendEvent, this);
		m_commandList.addEventCallback(onCommandListEvent, LV_EVENT_ALL, this);
		m_input.addEventCallback(onKeyboardEvent, LV_EVENT_ALL, this);
		m_commandVisibility.addClickedCallback(
			[](lv_event_t* e)
			{
				ConsoleView& view = *static_cast<ConsoleView*>(lv_event_get_user_data(e));
				bool show = !view.m_commandVisibility.hasState(LV_STATE_CHECKED);
				view.showCommandList(show, true);
			},
			this);

		showCommandList(!StorageHelper::getData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED, false), false);
	}

	void ConsoleView::clear()
	{
		UI_LOCK();
		lv_textarea_set_text(m_input, "");
	}

	void ConsoleView::addCommand(const char* resp)
	{
		addResponse(utils::format("> %s", resp).c_str());
	}

	void ConsoleView::addResponse(const char* resp)
	{
		m_output.addText(resp);
		m_output.addChar('\n');

		std::string_view currentText = m_output.getText();

		int newLineCount = std::count(currentText.begin(), currentText.end(), '\n');

		if (newLineCount > MAX_RESPONSE_LINES)
		{
			size_t pos = 0;
			for (int i = 0; i < newLineCount - MAX_RESPONSE_LINES; ++i)
			{
				pos = currentText.find('\n', pos) + 1;
			}
			currentText = currentText.substr(pos);
			m_output.setText(currentText.data());
		}
	}

	void ConsoleView::showCommandList(bool show, bool animate)
	{
		if (show == m_commandVisibility.hasState(LV_STATE_CHECKED))
		{
			return;
		}

		StorageHelper::setData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED, !show);
		m_commandVisibility.setChecked(show);

		int32_t start = show ? 1 : 20;
		int32_t end = show ? 20 : 1;

		if (animate)
		{
			LvAnim anim;
			anim.setDuration(animate ? 300 : 0);
			anim.setVar(this);
			anim.setValues(start, end);
			anim.setExecCb(
				[](void* var, int32_t value)
				{
					ConsoleView& view = *static_cast<ConsoleView*>(var);
					view.m_commandList.setFlexGrow(value);
					view.updateBtnPos();
				});
			anim.setDeletedCb(
				[](lv_anim_t* anim)
				{
					ConsoleView& view = *static_cast<ConsoleView*>(anim->var);
					view.m_commandList.setScrollDir(view.m_commandVisibility.hasState(LV_STATE_CHECKED) ? LV_DIR_ALL
																										: LV_DIR_VER);
					view.m_commandList.setFlexGrow(anim->end_value);
					view.updateBtnPos();
					view.m_output.updateLayout();
					view.m_output.setCursorPos(LV_TEXTAREA_CURSOR_LAST);
				}

			);
			anim.start();
		}
		else
		{
			m_commandList.setFlexGrow(end);
			updateBtnPos();
			m_output.updateLayout();
			m_output.setCursorPos(LV_TEXTAREA_CURSOR_LAST);
		}
	}

	void ConsoleView::showKeyboard(bool show)
	{
		m_kb.setTextArea(show ? &m_input : nullptr);
		m_kb.setMode(LV_KEYBOARD_MODE_TEXT_UPPER);
		m_kb.setVisible(show, true);
	}

	void ConsoleView::onSendEvent(lv_event_t* e)
	{
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		lv_obj_send_event(view->m_input, LV_EVENT_READY, view);
	}

	void ConsoleView::onClearEvent(lv_event_t* e)
	{
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		view->clear();
	}

	void ConsoleView::onCommandListEvent(lv_event_t* e)
	{
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		lv_event_code_t code = lv_event_get_code(e);
		if (code == LV_EVENT_VALUE_CHANGED)
		{
			uint32_t row;
			uint32_t col;
			lv_table_get_selected_cell(view->m_commandList, &row, &col);

			const char* gcode = lv_table_get_cell_value(view->m_commandList, row, 0);

			if (gcode[0] == '\0')
			{
				return;
			}
			lv_textarea_set_text(view->m_input, gcode);
		}
	}

	void ConsoleView::onKeyboardEvent(lv_event_t* e)
	{
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		lv_event_code_t code = lv_event_get_code(e);
		switch (code)
		{
		case LV_EVENT_FOCUSED:
		{
			view->showKeyboard(true);
			break;
		}
		case LV_EVENT_DEFOCUSED:
		{
			view->showKeyboard(false);
			break;
		}
		case LV_EVENT_VALUE_CHANGED:
		{
			lv_obj_scroll_to_y(view->m_commandList, 0, LV_ANIM_OFF);
			std::string cmd = lv_textarea_get_text(view->m_input);
			if (cmd.find_first_of(' ') == std::string::npos)
			{
				std::string upper_cmd;
				std::transform(cmd.begin(),
							   cmd.end(),
							   std::back_inserter(upper_cmd),
							   [](unsigned char c) { return std::toupper(c); });

				uint16_t index = 0;
				for (size_t i = 0; i < Gcodes::getGcodeCount(); i++)
				{
					const gcode* g = Gcodes::getGcode(i);
					if (std::string(g->gcode).rfind(upper_cmd, 0) == 0)
					{
						lv_table_set_cell_value(view->m_commandList, index, 0, g->gcode);
						lv_table_set_cell_value(view->m_commandList, index, 1, g->helpText);
						index++;
					}
				}

				lv_table_set_row_count(view->m_commandList, index);
			}

			break;
		}
		case LV_EVENT_READY:
		{
			const char* text = lv_textarea_get_text(view->m_input);
			if (strlen(text) > 0)
			{
				view->m_presenter->sendGcode(text);
			}
			break;
		}

		default:
			break;
		}

		if (code == LV_EVENT_CANCEL)
		{
			lv_obj_add_flag(view->m_kb, LV_OBJ_FLAG_HIDDEN);
		}
	}

	bool ConsoleView::back()
	{
		return false;
	}

	void ConsoleView::updateBtnPos()
	{
		m_commandList.updateLayout();
		m_commandVisibility.setPos(m_commandList.getX2() - m_commandVisibility.getWidth() - 5,
								   m_commandList.getY() + 5);
	}

	void ConsoleView::onShow()
	{
		m_commandList.scrollToX(0, LV_ANIM_OFF);
		m_kb.hide();
		updateBtnPos();
	}

	void ConsoleView::onHide()
	{
		// We want the console output to still update with new replies even when the console is hidden.
		activate();
	}
} // namespace UI

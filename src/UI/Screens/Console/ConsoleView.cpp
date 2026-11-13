#include "ConsoleView.h"
#include "Debug.h"
#include "Gcodes.h"
#include "Hardware/Duet.h"
#include "UI/Components/LVGL/LvAnim.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/TimeHelper.h"

#define AUTO_HIDE_KEYBOARD_ON_DEFOCUS 0

namespace UI
{
	static constexpr lv_coord_t s_gcodeWidth = 50;
	static constexpr lv_coord_t s_descriptionWidth = 500;
	static constexpr lv_coord_t s_inputBtnSize = 50;

	ConsoleView::ConsoleView(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		ZoneScoped;
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
		m_commandVisibility.setFlag(LV_OBJ_FLAG_FLOATING, true);
		m_commandVisibility.updateLayout();
		m_commandList.setFlexGrow(1);
		m_commandList.setMinWidth(0);
		m_commandList.setHeight(LV_PCT(100));
		m_commandVisibility.addEventCallback(
			[this](lv_event_t*)
			{
				const lv_coord_t pad = m_commandList.getStyleProp(LV_STYLE_PAD_LEFT).num +
									   m_commandList.getStyleProp(LV_STYLE_PAD_RIGHT).num;
				m_commandList.setMinWidth(m_commandVisibility.getWidth() + pad);
			},
			LV_EVENT_SIZE_CHANGED);
		m_commandVisibility.sendEvent(LV_EVENT_SIZE_CHANGED);

		m_outputCont.setFlexGrow(30);
		m_outputCont.setHeight(LV_PCT(100));
		m_outputCont.addStyle(Themes::getLvglStyles().card);
		m_output.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		// Command List
		m_commandList.getListContainer().setScrollDir(LV_DIR_ALL);
		m_commandList.setItemCount(Gcodes::getGcodeCount(),
								   [this](size_t index) { return std::make_unique<LazyGcodeItem>(index, *this); });

		// Input Area
		m_inputCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_inputCont.setFlexFlow(LV_FLEX_FLOW_ROW);

		m_input.setFlexGrow(1);
		m_input.setOneLine(true);
		m_input.setPlaceholderText(_("console.input_placeholder"));
		m_input.setStyleTextAlign(LV_TEXT_ALIGN_LEFT);
		m_input.setHeight(LV_SIZE_CONTENT);
		m_clear.setSize(s_inputBtnSize, s_inputBtnSize);
		m_enter.setSize(s_inputBtnSize, s_inputBtnSize);

		m_clear.setIcon("clear.png");
		m_enter.setIcon("send.png");

#if ENABLE_CONSOLE_SHELL
		m_shellToggle.setText(_("console.shell"));
		m_shellToggle.setFlag(LV_OBJ_FLAG_IGNORE_LAYOUT, true);
		m_shellToggle.setAlign(LV_ALIGN_TOP_RIGHT, -30, 20);
		m_shellToggle.setCheckedCallback(
			[this](bool checked)
			{
				getPresenter()->enableShell(checked);
				focusInput();
			});
		m_shellToggle.setChecked(false);
#endif

		m_topCont.addStyle(Themes::getLvglStyles().no_border);
		m_inputCont.addStyle(Themes::getLvglStyles().no_border);

		// Hide keyboard initially
		m_kb.setSize(LV_PCT(100), LV_PCT(40));
		m_kb.setMode(LV_KEYBOARD_MODE_TEXT_UPPER);
		m_kb.hide();

		// Callbacks
		m_clear.addClickedCallback(onClearEvent, this);
		m_enter.addClickedCallback(onSendEvent, this);
		m_input.addEventCallback(onKeyboardEvent, LV_EVENT_ALL, this);
		m_commandVisibility.addClickedCallback(
			[](lv_event_t* e)
			{
				ConsoleView& view = *static_cast<ConsoleView*>(lv_event_get_user_data(e));
				bool show = !view.m_commandVisibility.hasState(LV_STATE_CHECKED);
				view.showCommandList(show, true);

				view.focusInput();
			},
			this);

		m_commandList.getListContainer().hide();
		showCommandList(!StorageHelper::getData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED), LV_ANIM_OFF);
	}

	void ConsoleView::clear()
	{
		ZoneScoped;
		m_input.setText("");
	}

	void ConsoleView::addCommand(std::string_view resp)
	{
		ZoneScoped;
		addResponse(fmt::format("> {:s}", resp), true);
	}

	void ConsoleView::addResponse(const std::string& resp, const bool emphasize)
	{
		ZoneScoped;
#if !USE_FIXED_TEST_STRINGS
		const auto now = std::chrono::system_clock::now();
#endif

		const auto timePrefix = fmt::format("[{:02}:{:02}:{:02}]   ",
#if USE_FIXED_TEST_STRINGS
											12,
											34,
											56
#else
											std::chrono::duration_cast<std::chrono::hours>(now.time_since_epoch())
													.count() %
												24,
											std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch())
													.count() %
												60,
											std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
													.count() %
												60
#endif
		);
		auto timeSpan = m_output.addSpan();
		timeSpan.setText(timePrefix);
		m_output.setSpanStyleStatic(timeSpan, Themes::getLvglStyles().text_muted);

		auto respSpan = m_output.addSpan();
		respSpan.setText(resp);
		m_output.setSpanStyleStatic(respSpan,
									emphasize ? Themes::getLvglStyles().text_emphasis : Themes::getLvglStyles().text);

		if (resp.length() > 0 && resp.back() != '\n')
		{
			auto newlineSpan = m_output.addSpan();
			newlineSpan.setText("\n");
		}

		while (m_output.getSpanCount() > MAX_RESPONSE_LINES * 3) // 3 spans per line (most of the time)
		{
			auto spanOpt = m_output.getSpanByIndex(0);
			if (!spanOpt.has_value())
			{
				break;
			}
			m_output.deleteSpan(std::move(spanOpt.value()));
		}

		if (isVisible())
		{
			m_output.updateLayout();
			m_outputCont.scrollByBounded(0, -m_outputCont.getScrollBottom(), LV_ANIM_OFF);
		}
	}

	ConsoleView::~ConsoleView()
	{
		lv_anim_delete(this, nullptr);
	}

	void ConsoleView::showCommandList(bool show, bool animate)
	{
		ZoneScoped;
		if (show == m_commandVisibility.hasState(LV_STATE_CHECKED))
		{
			return;
		}

		StorageHelper::setData(ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED, !show);
		m_commandVisibility.setChecked(show);

		uint8_t start = show ? 1 : 10;
		uint8_t end = show ? 10 : 1;

		if (animate)
		{
			LvAnim anim;
			anim.setDuration(animate ? 300 : 0);
			anim.setVar(this);
			anim.setValues(start, end);
			anim.setExecCb(
				[](void* var, int32_t value)
				{
					ZoneScopedN("ConsoleView::showCommandList:animExecCb");
					ConsoleView& view = *static_cast<ConsoleView*>(var);
					view.m_commandList.setFlexGrow(static_cast<uint8_t>(value));
					view.updateBtnPos();
				});
			anim.setDeletedCb(
				[](lv_anim_t* anim)
				{
					ZoneScopedN("ConsoleView::showCommandList:animDeletedCb");
					ConsoleView& view = *static_cast<ConsoleView*>(anim->var);
					view.m_commandList.setFlexGrow(static_cast<uint8_t>(anim->end_value));
					view.m_commandList.getListContainer().setVisible(anim->end_value > 1);
					view.updateBtnPos();
					view.m_output.updateLayout();
				}

			);
			if (show)
			{
				m_commandList.getListContainer().show();
			}
			anim.start();
		}
		else
		{
			m_commandList.setFlexGrow(static_cast<uint8_t>(end));
			m_commandList.getListContainer().setVisible(show);
			updateBtnPos();
			m_output.updateLayout();
		}
	}

	void ConsoleView::showKeyboard(bool show)
	{
		ZoneScoped;
		m_kb.setTextarea(show ? &m_input : nullptr);
		m_kb.setMode(LV_KEYBOARD_MODE_TEXT_UPPER);
		m_kb.setVisible(show, true);
	}

	void ConsoleView::onSendEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		view->m_outputCont.scrollByBounded(0, -view->m_outputCont.getScrollBottom(), LV_ANIM_ON);
		view->m_input.sendEvent(LV_EVENT_READY, view);
		view->focusInput();
	}

	void ConsoleView::onClearEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		view->clear();
		view->focusInput();
	}

#if 0
	void ConsoleView::onCommandListEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		lv_event_code_t code = lv_event_get_code(e);
		if (code == LV_EVENT_VALUE_CHANGED)
		{
			ZoneScopedN("ConsoleView::onCommandListEvent:LV_EVENT_VALUE_CHANGED");
			uint32_t row;
			uint32_t col;
			view->m_commandList.getSelectedCell(&row, &col);

			const char* gcode = view->m_commandList.getCellValue(row, 0);

			if (gcode[0] == '\0')
			{
				return;
			}
			view->m_input.setText(gcode);
		}
	}
#endif

	void ConsoleView::onKeyboardEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		ConsoleView* view = static_cast<ConsoleView*>(lv_event_get_user_data(e));
		lv_event_code_t code = lv_event_get_code(e);
		switch (code)
		{
		case LV_EVENT_FOCUSED:
		{
			ZoneScopedN("ConsoleView::onKeyboardEvent:LV_EVENT_FOCUSED");
			view->showKeyboard(true);
			break;
		}
#if AUTO_HIDE_KEYBOARD_ON_DEFOCUS
		case LV_EVENT_DEFOCUSED:
		{
			ZoneScopedN("ConsoleView::onKeyboardEvent:LV_EVENT_DEFOCUSED");
			view->showKeyboard(false);
			break;
		}
#endif
		case LV_EVENT_VALUE_CHANGED:
		{
			ZoneScopedN("ConsoleView::onKeyboardEvent:LV_EVENT_VALUE_CHANGED");
			view->m_commandList.scrollToY(0, LV_ANIM_OFF);

			std::string_view cmd = view->m_input.getText();
			cmd = cmd.substr(0, cmd.find_first_of(' '));

			LOG_DBG("Filtering command list for '{}'", cmd);
			std::string upper_cmd;
			std::transform(
				cmd.begin(), cmd.end(), std::back_inserter(upper_cmd), [](unsigned char c) { return std::toupper(c); });

			for (size_t i = 0; i < Gcodes::getGcodeCount(); i++)
			{
				const gcode* g = Gcodes::getGcode(i);
				const bool visible = g->gcode.rfind(upper_cmd, 0) == 0;
				view->m_commandList.getLazyItem(i)->setVisible(visible);
			}
			view->m_commandList.refresh();
			break;
		}
		case LV_EVENT_READY:
		{
			ZoneScopedN("ConsoleView::onKeyboardEvent:LV_EVENT_READY");
			std::string_view text = view->m_input.getText();
			if (!text.empty())
			{
				view->m_presenter->sendCommand(text);
			}
			break;
		}

		default:
			break;
		}

		if (code == LV_EVENT_CANCEL)
		{
			ZoneScopedN("ConsoleView::onKeyboardEvent:LV_EVENT_CANCEL");
			view->m_kb.hide();
		}
	}

	bool ConsoleView::back()
	{
		ZoneScoped;
		return false;
	}

	void ConsoleView::updateBtnPos()
	{
		ZoneScoped;
		m_commandList.updateLayout();
		const lv_coord_t pad_right = m_commandList.getStyleProp(LV_STYLE_PAD_RIGHT).num;
		const lv_coord_t pad_top = m_commandList.getStyleProp(LV_STYLE_PAD_TOP).num;
		m_commandVisibility.setPos(m_commandList.getX2() - m_commandVisibility.getWidth() - pad_right,
								   m_commandList.getY() + pad_top);
	}

	void ConsoleView::focusInput()
	{
		ZoneScoped;
		const bool kbVisible = m_kb.isVisible();
		lv_group_focus_obj(m_input.getRootPtr());
		m_kb.setVisible(kbVisible);
	}

	void ConsoleView::onShow()
	{
		ZoneScoped;
		m_commandList.scrollToX(0, LV_ANIM_OFF);
		m_outputCont.scrollByBounded(0, -m_outputCont.getScrollBottom(), LV_ANIM_ON);
		m_kb.hide();
		focusInput();
		updateBtnPos();
	}

	void ConsoleView::onHide()
	{
		ZoneScoped;
		// We want the console output to still update with new replies even when the console is hidden.
		activate();
	}

	ConsoleView::GcodeItem::GcodeItem(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		setWidth(LV_SIZE_CONTENT);
		m_gcode.setWidth(s_gcodeWidth);
		m_gcode.setMinWidth(LV_SIZE_CONTENT);
		m_description.setWidth(LV_SIZE_CONTENT);
		m_description.addStyle(Themes::getLvglStyles().text_muted);

		addEventCallback(
			[this](lv_event_t*)
			{
				if (m_consoleView)
				{
					m_consoleView->m_input.setText(m_gcode.getText());
				}
			},
			LV_EVENT_CLICKED);
	}

	ConsoleView::LazyGcodeItem::LazyGcodeItem(size_t index, ConsoleView& view)
		: m_index(index)
		, m_view(view)
	{
		ZoneScoped;
	}

	lv_coord_t ConsoleView::LazyGcodeItem::getWidth() const
	{
		ZoneScoped;
		return 0;
	}

	lv_coord_t ConsoleView::LazyGcodeItem::getHeight() const
	{
		ZoneScoped;
		return 30;
	}

	void ConsoleView::LazyGcodeItem::update(size_t index, ConsoleView::GcodeItem& obj)
	{
		ZoneScoped;
		const gcode* g = Gcodes::getGcode(index);

		obj.setGcode(g->gcode);
		obj.setDescription(g->helpText);
		obj.setConsoleView(&m_view);
	}
} // namespace UI

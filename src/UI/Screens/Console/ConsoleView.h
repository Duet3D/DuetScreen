#pragma once

#include "ConsolePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvCheckbox.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/LVGL/LvSpan.h"
#include "UI/Components/LVGL/LvTextarea.h"
#include "UI/Components/List/LazyList.h"
#include "UI/Core/View.h"

namespace UI
{
	class ConsoleView : public View<ConsolePresenter>
	{
	  public:
		class GcodeItem : public LvContainer
		{
			friend class LazyGcodeItem;

		  public:
			GcodeItem(const std::string& name, LvObj& parent);

			void setGcode(std::string_view gcode) { m_gcode.setText(gcode); }
			void setDescription(std::string_view description) { m_description.setText(description); }
			void setConsoleView(ConsoleView* view) { m_consoleView = view; }

		  private:
			LvLabel m_gcode{"gcode", getRoot()};
			LvLabel m_description{"description", getRoot()};

			ConsoleView* m_consoleView = nullptr;
		};

		class LazyGcodeItem : public LazyObj<GcodeItem>
		{
		  public:
			LazyGcodeItem(size_t index, ConsoleView& view);

			lv_coord_t getWidth() const override;
			lv_coord_t getHeight() const override;
			void update(size_t index, GcodeItem& obj) override;
			bool isVisible() const override { return m_visible; }

			void setVisible(bool visible) { m_visible = visible; }

		  private:
			size_t m_index;
			ConsoleView& m_view;

			bool m_visible = true;
		};
		friend class GcodeItem;
		friend class ConsolePresenter;

		ConsoleView(const std::string& name, LvObj& parent);

		/** Stop the command list animation, which works on this object */
		~ConsoleView() override;

		void clear();
		void addCommand(std::string_view resp);
		void addResponse(const std::string& resp, const bool emphasize = false);
		void showCommandList(bool show, bool animate = LV_ANIM_ON);
		void showKeyboard(bool show);

		bool back() override;

	  private:
		static void onSendEvent(lv_event_t* e);
		static void onClearEvent(lv_event_t* e);
		static void onCommandListEvent(lv_event_t* e);
		static void onKeyboardEvent(lv_event_t* e);

		void updateBtnPos();
		void focusInput();

		void onShow() override;
		void onHide() override;

		LvContainer m_topCont{"top_cont", getRoot()};
		LazyList<LazyGcodeItem> m_commandList{"command_list", m_topCont};
		Button m_commandVisibility{"command_visibility", m_topCont, LV_SYMBOL_LIST};
		LvContainer m_outputCont{"output_cont", m_topCont};
		LvSpanGroup m_output{"output", m_outputCont};
		LvContainer m_inputCont{"input_cont", getRoot()};
		LvTextarea m_input{"input", m_inputCont};
		Button m_clear{"clear", m_inputCont};
		Button m_enter{"enter", m_inputCont};
		LvKeyboard m_kb{"keyboard", getRoot()};

#if ENABLE_CONSOLE_SHELL
		LvCheckbox m_shellToggle{"shell_toggle", getRoot()};
#endif
	};
} // namespace UI
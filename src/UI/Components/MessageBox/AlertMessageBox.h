/*
 * AlertMessageBox.h
 *
 *  Created on: 2025-08-12
 *      Author: Andy Everitt
 */

#pragma once

#include "MessageBox.h"
#include "ObjectModel/Alert.h"
#include "ObjectModel/Axis.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/LVGL/LvTextArea.h"
#include "UI/Components/List/List.h"
#include <functional>
#include <map>
#include <memory>

namespace UI
{
    class AlertMessageBox : public MessageBox
	{

	  public:
		class AxisJog : public LvObj
		{
		  public:
			AxisJog(const size_t index, LvObj& parent, AlertMessageBox& msgBox);
			void setAxisLetter(char letter);
			const char* getAxisLetter() const { return m_axisLetter; }
			void setPosition(float position);
			void setEnabled(bool enabled);

		  private:
			static void onRelMoveEvent(lv_event_t* e);

			const size_t m_index;
			AlertMessageBox& m_msgBox;

			char m_axisLetter[2];
			float m_position;

			LvLabel m_label;
			Button m_relMove[6];
		};

		friend class AxisJog;

		AlertMessageBox(const std::string& name, LvObj& parent, layout_t layout);
		virtual ~AlertMessageBox() = default;

		void setMode(OM::Alert::Mode mode);
        
		void setChoiceCallback(std::function<void(size_t)> cb) { m_choiceCb = cb; }
		void setInputValidationCallback(std::function<bool(std::string_view)> cb);
		void setShowKeyboardCallback(std::function<void(bool)> cb) { m_showKeyboardCb = cb; }

		void setKeyboard(LvKeyboard* keyboard);

		bool isBlocking() const;
		bool isResponse() const;
        void clear();

		void setMinText(std::string_view format);
		void setMaxText(std::string_view format);
		void setWarningText(std::string_view format);
		void setInput(int32_t val);
		void setInput(float val);
		void setInput(std::string_view text);

		void selectionVisible(bool visible);
		void inputVisible(bool visible);
		void warningTextVisible(bool visible);
		void minTextVisible(bool visible);
		void maxTextVisible(bool visible);
		void axisJogVisible(bool visible);

		// Axis Jog
		size_t getJogAxisCount() const;
		void setJogAxisCount(size_t count);
		std::string_view getJogAxisLetter(size_t index) const;
		void setJogAxisLetter(size_t index, char letter);
		void setJogAxisPosition(size_t index, float position);
		void setJogAxisEnabled(size_t index, bool enabled);

		// Choices
		size_t getChoiceCount() const;
		void setChoiceCount(size_t count);
		void setChoice(size_t index, std::string_view text);

		std::string_view getInput() const;

		bool validate();

		bool validateIntegerInput(std::string_view text);
		bool validateFloatInput(std::string_view text);
		bool validateTextInput(std::string_view text);

		const OM::Alert::Mode getMode() const { return m_mode; }

	  private:
		static void onChoiceEvent(lv_event_t* e);
		static void onInputEvent(lv_event_t* e);

		void init();
		bool validateIntegerInputInner(std::string_view text);
		bool validateFloatInputInner(std::string_view text);
		bool validateTextInputInner(std::string_view text);

		// Top container

		// Central Container
		LvContainer m_inputCont;
        
		List<AxisJog> m_axisJogList;
		List<Button> m_choicesList;

		// Input Container
		LvLabel m_warningText;
		LvLabel m_minText;
		LvLabel m_maxText;
		LvTextArea m_input;

		LvKeyboard* m_kb; // Keyboard

		std::function<void(size_t)> m_choiceCb;
		std::function<bool(std::string_view)> m_inputValidationCb;
		std::function<void(bool)> m_showKeyboardCb;
		OM::Alert::Mode m_mode = OM::Alert::Mode::None;
	};
} // namespace UI

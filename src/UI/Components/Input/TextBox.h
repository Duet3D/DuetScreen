/*
 * TextBox.h
 *
 *  Created on: 2025-05-08
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvObj.h"
#include "UI/Components/LVGL/LvTextarea.h"

namespace UI
{
	class TextBox : public LvContainer
	{
	  public:
		enum class Mode
		{
			TEXT,
			NUMBER
		};

		TextBox(const std::string& name, LvObj& parent);
		TextBox(const std::string& name, LvObj& parent, layout_t layout);

		void setLabel(const std::string& label);
		void setText(const std::string& text);
		std::string_view getText() const;
		LvTextarea& getTextarea() { return m_textarea; }

		void addChar(uint32_t c);
		void addText(const std::string& text);
		void deleteChar();
		void deleteCharForward();
		void setPlaceholderText(const std::string& text);
		void setCursorPos(int32_t pos);
		void setCursorClickPos(bool clickPos);
		void setPasswordMode(bool passwordMode);
		void setPasswordBullet(const char* bullet);
		void setPasswordShowTime(uint32_t time);
		void setOneLine(bool oneLine);
		void setAcceptedChars(const char* chars);
		void setMaxLength(uint32_t length);
		void setTextSelection(bool enable);

		const char* getPlaceholderText() const;
		uint32_t getCursorPos() const;
		bool getCursorClickPosEnabled() const;
		bool getPasswordModeEnabled() const;
		const char* getPasswordBullet() const;
		bool getOneLineEnabled() const;
		const char* getAcceptedChars() const;
		uint32_t getMaxLength() const;
		bool isTextSelected() const;
		bool getTextSelectionEnabled() const;
		uint32_t getPasswordShowTime() const;
		uint32_t getCurrentChar() const;

		void clearSelection();
		void cursorRight();
		void cursorLeft();
		void cursorUp();
		void cursorDown();

		void addConfirmEventCallback(std::function<void(lv_event_t*)> cb);

		void showPassword(bool show);

		void setMode(Mode mode) { m_mode = mode; }
		Mode getMode() const { return m_mode; }

		void setKeyboard(LvKeyboard* keyboard) { m_keyboard = keyboard; }
		LvKeyboard* getKeyboard() const { return m_keyboard; }


	  private:
		void init();

		LvLabel m_label{"label", getRoot()};
		LvTextarea m_textarea{"textarea", getRoot()};
		Button m_showPassword{"show_password", getRoot(), LV_SYMBOL_EYE_OPEN};

		LvKeyboard* m_keyboard = nullptr;

		Mode m_mode = Mode::TEXT;
		bool m_passwordMode = false;
	};
} // namespace UI

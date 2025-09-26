/*
 * LvTextArea.h
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
    class LvTextArea : public LvObj
    {
      public:
		LvTextArea(const std::string& name, LvObj& parent);

		void setText(std::string_view text);
		std::string_view getText() const;
		lv_obj_t* getTextArea() const { return getRoot(); }

		void addChar(uint32_t c);
		void addText(const std::string& text);
		void deleteChar();
		void deleteCharForward();
		void setPlaceholderText(const std::string& text);
		void setCursorPos(uint32_t pos);
		void setCursorClickPos(bool clickPos);
		void setPasswordMode(bool passwordMode);
		void setPasswordBullet(const char* bullet);
		void setPasswordShowTime(uint32_t time);
		void setOneLine(bool oneLine);
		void setAcceptedChars(const char* chars);
		void setMaxLength(uint32_t length);
		void setTextSelection(bool enable);

		const char* getPlaceholderText() const;
		lv_obj_t* getLabel() const;
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

	  private:
    };
} // namespace UI

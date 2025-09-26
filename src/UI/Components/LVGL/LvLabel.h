/*
 * LvLabel.h
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Core/View.h"

namespace UI
{
	class LvLabel : public LvObj
	{
	  public:
		LvLabel(const std::string& name, LvObj& parent);
		LvLabel(const std::string& name, LvObj& parent, const std::string& text);

		void setText(const std::string_view text);
		void setTextF(const char* fmt, ...);
		void setTextStatic(const char* text);
		void setLongMode(lv_label_long_mode_t mode);
		void setTextSelectionStart(uint32_t index);
		void setTextSelectionEnd(uint32_t index);
		void setRecolor(bool enable);

		std::string_view getText() const;
		lv_label_long_mode_t getLongMode() const;
		void getLetterPos(uint32_t char_id, lv_point_t* pos) const;
		uint32_t getLetterOn(lv_point_t* pos_in, bool bidi = false) const;
		bool isCharUnderPos(lv_point_t* pos) const;
		uint32_t getTextSelectionStart() const;
		uint32_t getTextSelectionEnd() const;
		bool isRecolorEnabled() const;

		void insertText(uint32_t pos, const std::string& text);
		void cutText(uint32_t pos, uint32_t len);

	  private:
		void init();
	};
} // namespace UI

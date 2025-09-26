/*
 * LvLabel.cpp
 *
 *  Created on: 2025-06-10
 *      Author: Andy Everitt
 */

#include "LvLabel.h"
#include "Debug.h"

namespace UI
{
	LvLabel::LvLabel(const std::string& name, LvObj& parent)
		: LvObj(lv_label_create, name, parent)
	{
		UI_LOCK();
		init();
	}

	LvLabel::LvLabel(const std::string& name, LvObj& parent, const std::string& text)
		: LvObj(lv_label_create, name, parent)
	{
		UI_LOCK();
		init();
		setText(text);
	}

	void LvLabel::init()
	{
		setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	}

	void LvLabel::setText(const std::string_view text)
	{
		UI_LOCK();
		if (text.data() == nullptr)
		{
			lv_label_set_text(getRoot(), nullptr);
			return;
		}
		lv_label_set_text_fmt(getRoot(), "%.*s", (int)text.length(), text.data());
	}

	void LvLabel::setTextF(const char* fmt, ...)
	{
		UI_LOCK();
		va_list args;
		va_start(args, fmt);
		lv_label_set_text_fmt(getRoot(), fmt, args);
		va_end(args);
	}

	void LvLabel::setTextStatic(const char* text)
	{
		UI_LOCK();
		lv_label_set_text_static(getRoot(), text);
	}

	void LvLabel::setLongMode(lv_label_long_mode_t mode)
	{
		UI_LOCK();
		lv_label_set_long_mode(getRoot(), mode);
	}

	void LvLabel::setTextSelectionStart(uint32_t index)
	{
		UI_LOCK();
		lv_label_set_text_selection_start(getRoot(), index);
	}

	void LvLabel::setTextSelectionEnd(uint32_t index)
	{
		UI_LOCK();
		lv_label_set_text_selection_end(getRoot(), index);
	}

	void LvLabel::setRecolor(bool enable)
	{
		UI_LOCK();
		lv_label_set_recolor(getRoot(), enable);
	}

	std::string_view LvLabel::getText() const
	{
		UI_LOCK();
		return lv_label_get_text(getRoot());
	}

	lv_label_long_mode_t LvLabel::getLongMode() const
	{
		UI_LOCK();
		return lv_label_get_long_mode(getRoot());
	}

	void LvLabel::getLetterPos(uint32_t char_id, lv_point_t* pos) const
	{
		UI_LOCK();
		lv_label_get_letter_pos(getRoot(), char_id, pos);
	}

	uint32_t LvLabel::getLetterOn(lv_point_t* pos_in, bool bidi) const
	{
		UI_LOCK();
		return lv_label_get_letter_on(getRoot(), pos_in, bidi);
	}

	bool LvLabel::isCharUnderPos(lv_point_t* pos) const
	{
		UI_LOCK();
		return lv_label_is_char_under_pos(getRoot(), pos);
	}

	uint32_t LvLabel::getTextSelectionStart() const
	{
		UI_LOCK();

		return lv_label_get_text_selection_start(getRoot());
	}

	uint32_t LvLabel::getTextSelectionEnd() const
	{
		UI_LOCK();
		return lv_label_get_text_selection_end(getRoot());
	}

	bool LvLabel::isRecolorEnabled() const
	{
		UI_LOCK();
		return lv_label_get_recolor(getRoot());
	}

	void LvLabel::insertText(uint32_t pos, const std::string& text)
	{
		UI_LOCK();
		lv_label_ins_text(getRoot(), pos, text.c_str());
	}

	void LvLabel::cutText(uint32_t pos, uint32_t len)
	{
		UI_LOCK();
		lv_label_cut_text(getRoot(), pos, len);
	}

} // namespace UI

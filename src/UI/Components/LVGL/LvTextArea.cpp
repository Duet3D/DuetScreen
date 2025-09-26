/*
 * LvTextArea.cpp
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#include "LvTextArea.h"
#include "Debug.h"

namespace UI
{
	LvTextArea::LvTextArea(const std::string& name, LvObj& parent)
		: LvObj(lv_textarea_create, name, parent)
	{
		UI_LOCK();

		setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		setCursorClickPos(true);
	}

	void LvTextArea::setText(std::string_view text)
	{
		UI_LOCK();
		lv_textarea_set_text(getTextArea(), text.data());
	}

	std::string_view LvTextArea::getText() const
	{
		UI_LOCK();
		return lv_textarea_get_text(getTextArea());
	}

	void LvTextArea::addChar(uint32_t c)
	{
		UI_LOCK();
		lv_textarea_add_char(getTextArea(), c);
	}

	void LvTextArea::addText(const std::string& text)
	{
		UI_LOCK();
		lv_textarea_add_text(getTextArea(), text.c_str());
	}

	void LvTextArea::deleteChar()
	{
		UI_LOCK();
		lv_textarea_delete_char(getTextArea());
	}

	void LvTextArea::deleteCharForward()
	{
		UI_LOCK();
		lv_textarea_delete_char_forward(getTextArea());
	}

	void LvTextArea::setPlaceholderText(const std::string& text)
	{
		UI_LOCK();
		lv_textarea_set_placeholder_text(getTextArea(), text.c_str());
	}

	void LvTextArea::setCursorPos(uint32_t pos)
	{
		UI_LOCK();
		lv_textarea_set_cursor_pos(getTextArea(), pos);
	}

	void LvTextArea::setCursorClickPos(bool clickPos)
	{
		UI_LOCK();
		lv_textarea_set_cursor_click_pos(getTextArea(), clickPos);
	}

	void LvTextArea::setPasswordMode(bool passwordMode)
	{
		UI_LOCK();
		lv_textarea_set_password_mode(getTextArea(), passwordMode);
	}

	void LvTextArea::setPasswordBullet(const char* bullet)
	{
		UI_LOCK();
		lv_textarea_set_password_bullet(getTextArea(), bullet);
	}

	void LvTextArea::setPasswordShowTime(uint32_t time)
	{
		UI_LOCK();
		lv_textarea_set_password_show_time(getTextArea(), time);
	}

	void LvTextArea::setOneLine(bool oneLine)
	{
		UI_LOCK();
		lv_textarea_set_one_line(getTextArea(), oneLine);
	}
	void LvTextArea::setAcceptedChars(const char* chars)
	{
		UI_LOCK();
		lv_textarea_set_accepted_chars(getTextArea(), chars);
	}
	void LvTextArea::setMaxLength(uint32_t length)
	{
		UI_LOCK();
		lv_textarea_set_max_length(getTextArea(), length);
	}

	void LvTextArea::setTextSelection(bool enable)
	{
		UI_LOCK();
		lv_textarea_set_text_selection(getTextArea(), enable);
	}

	const char* LvTextArea::getPlaceholderText() const
	{
		UI_LOCK();
		return lv_textarea_get_placeholder_text(getTextArea());
	}

	lv_obj_t* LvTextArea::getLabel() const
	{
		UI_LOCK();
		return lv_textarea_get_label(getTextArea());
	}

	uint32_t LvTextArea::getCursorPos() const
	{
		UI_LOCK();
		return lv_textarea_get_cursor_pos(getTextArea());
	}

	bool LvTextArea::getCursorClickPosEnabled() const
	{
		UI_LOCK();
		return lv_textarea_get_cursor_click_pos(getTextArea());
	}

	bool LvTextArea::getPasswordModeEnabled() const
	{
		UI_LOCK();
		return lv_textarea_get_password_mode(getTextArea());
	}

	const char* LvTextArea::getPasswordBullet() const
	{
		UI_LOCK();
		return lv_textarea_get_password_bullet(getTextArea());
	}

	bool LvTextArea::getOneLineEnabled() const
	{
		UI_LOCK();
		return lv_textarea_get_one_line(getTextArea());
	}

	const char* LvTextArea::getAcceptedChars() const
	{
		UI_LOCK();
		return lv_textarea_get_accepted_chars(getTextArea());
	}

	uint32_t LvTextArea::getMaxLength() const
	{
		UI_LOCK();
		return lv_textarea_get_max_length(getTextArea());
	}

	bool LvTextArea::isTextSelected() const
	{
		UI_LOCK();
		return lv_textarea_text_is_selected(getTextArea());
	}

	bool LvTextArea::getTextSelectionEnabled() const
	{
		UI_LOCK();
		return lv_textarea_get_text_selection(getTextArea());
	}

	uint32_t LvTextArea::getPasswordShowTime() const
	{
		UI_LOCK();
		return lv_textarea_get_password_show_time(getTextArea());
	}

	uint32_t LvTextArea::getCurrentChar() const
	{
		UI_LOCK();
		return lv_textarea_get_current_char(getTextArea());
	}

	void LvTextArea::clearSelection()
	{
		UI_LOCK();
		lv_textarea_clear_selection(getTextArea());
	}

	void LvTextArea::cursorRight()
	{
		UI_LOCK();
		lv_textarea_cursor_right(getTextArea());
	}

	void LvTextArea::cursorLeft()
	{
		UI_LOCK();
		lv_textarea_cursor_left(getTextArea());
	}

	void LvTextArea::cursorUp()
	{
		UI_LOCK();
		lv_textarea_cursor_up(getTextArea());
	}

	void LvTextArea::cursorDown()
	{
		UI_LOCK();
		lv_textarea_cursor_down(getTextArea());
	}
} // namespace UI

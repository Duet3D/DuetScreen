/*
 * TextBox.cpp
 *
 *  Created on: 2025-05-08
 *      Author: Andy Everitt
 */

#include "TextBox.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"

namespace UI
{
	TextBox::TextBox(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_label("label", *this)
		, m_textArea("textarea", *this)
		, m_showPassword("show_password", m_textArea, LV_SYMBOL_EYE_OPEN)
	{
		init();
	}

	TextBox::TextBox(const std::string& name, LvObj& parent, layout_t layout)
		: LvObj(lv_obj_create, name, parent, layout)
		, m_label("label", getRoot())
		, m_textArea("textarea", getRoot())
		, m_showPassword("show_password", m_textArea, LV_SYMBOL_EYE_OPEN)
	{
		init();
	}

	void TextBox::init()
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(getRoot(), LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		// Label
		m_label.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_label.setMaxWidth(LV_PCT(50));
		setLabel("");

		// TextArea
		m_textArea.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		// m_textArea.setMinHeight(20);
		m_textArea.setFlexGrow(1);
		m_textArea.setCursorClickPos(true);

		// Show Password Button
		m_showPassword.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_showPassword.setAlign(LV_ALIGN_RIGHT_MID, 0, 0);
		m_showPassword.hide();
		m_showPassword.setCheckable(true);
		m_showPassword.addClickedCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				TextBox* tb = (TextBox*)lv_event_get_user_data(e);

				bool passwordMode = lv_textarea_get_password_mode(tb->m_textArea);
				tb->showPassword(passwordMode);
				// lv_group_focus_obj(tb->m_textArea);
			},
			this);

		m_textArea.addStyle(Themes::getLvglStyles().input);
	}

	void TextBox::setLabel(const std::string& label)
	{
		m_label.setFlag(LV_OBJ_FLAG_HIDDEN, label.empty());
		m_label.setText(label);
	}
	void TextBox::setText(std::string_view text)
	{
		m_textArea.setText(text);
		m_textArea.setCursorPos(0);
		m_textArea.scrollToX(0, LV_ANIM_OFF);
	}
	std::string_view TextBox::getText() const
	{
		return m_textArea.getText();
	}

	void TextBox::addChar(uint32_t c)
	{
		m_textArea.addChar(c);
	}

	void TextBox::addText(const std::string& text)
	{
		m_textArea.addText(text);
	}

	void TextBox::deleteChar()
	{
		m_textArea.deleteChar();
	}

	void TextBox::deleteCharForward()
	{
		m_textArea.deleteCharForward();
	}

	void TextBox::setPlaceholderText(const std::string& text)
	{
		m_textArea.setPlaceholderText(text);
	}

	void TextBox::setCursorPos(uint32_t pos)
	{
		m_textArea.setCursorPos(pos);
	}

	void TextBox::setCursorClickPos(bool clickPos)
	{
		m_textArea.setCursorClickPos(clickPos);
	}

	void TextBox::setPasswordMode(bool passwordMode)
	{
		m_passwordMode = passwordMode;
		m_showPassword.setChecked(false);
		m_showPassword.setVisible(passwordMode);
		m_textArea.setPasswordMode(passwordMode);
	}

	void TextBox::setPasswordBullet(const char* bullet)
	{
		m_textArea.setPasswordBullet(bullet);
	}

	void TextBox::setPasswordShowTime(uint32_t time)
	{
		m_textArea.setPasswordShowTime(time);
	}

	void TextBox::showPassword(bool show)
	{
		UI_LOCK();
		if (!m_passwordMode)
			return;
		m_textArea.setPasswordMode(!show);
		m_showPassword.setChecked(show);
	}

	void TextBox::setOneLine(bool oneLine)
	{
		m_textArea.setOneLine(oneLine);
	}
	void TextBox::setAcceptedChars(const char* chars)
	{
		m_textArea.setAcceptedChars(chars);
	}
	void TextBox::setMaxLength(uint32_t length)
	{
		m_textArea.setMaxLength(length);
	}

	void TextBox::setTextSelection(bool enable)
	{
		m_textArea.setTextSelection(enable);
	}

	const char* TextBox::getPlaceholderText() const
	{
		return m_textArea.getPlaceholderText();
	}

	uint32_t TextBox::getCursorPos() const
	{
		return m_textArea.getCursorPos();
	}

	bool TextBox::getCursorClickPosEnabled() const
	{
		return m_textArea.getCursorClickPosEnabled();
	}

	bool TextBox::getPasswordModeEnabled() const
	{
		return m_textArea.getPasswordModeEnabled();
	}

	const char* TextBox::getPasswordBullet() const
	{
		return m_textArea.getPasswordBullet();
	}

	bool TextBox::getOneLineEnabled() const
	{
		return m_textArea.getOneLineEnabled();
	}

	const char* TextBox::getAcceptedChars() const
	{
		return m_textArea.getAcceptedChars();
	}

	uint32_t TextBox::getMaxLength() const
	{
		return m_textArea.getMaxLength();
	}

	bool TextBox::isTextSelected() const
	{
		return m_textArea.isTextSelected();
	}

	bool TextBox::getTextSelectionEnabled() const
	{
		return m_textArea.getTextSelectionEnabled();
	}

	uint32_t TextBox::getPasswordShowTime() const
	{
		return m_textArea.getPasswordShowTime();
	}

	uint32_t TextBox::getCurrentChar() const
	{
		return m_textArea.getCurrentChar();
	}

	void TextBox::clearSelection()
	{
		m_textArea.clearSelection();
	}

	void TextBox::cursorRight()
	{
		m_textArea.cursorRight();
	}

	void TextBox::cursorLeft()
	{
		m_textArea.cursorLeft();
	}

	void TextBox::cursorUp()
	{
		m_textArea.cursorUp();
	}

	void TextBox::cursorDown()
	{
		m_textArea.cursorDown();
	}

	void TextBox::addConfirmEventCallback(lv_event_cb_t cb, void* userData)
	{
		UI_LOCK();
		lv_obj_set_user_data(m_textArea, reinterpret_cast<void*>(cb));
		m_textArea.addEventCallback(
			[](lv_event_t* e)
			{
				UI_LOCK();
				auto callback =
					reinterpret_cast<lv_event_cb_t>(lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e)));
				lv_event_code_t code = lv_event_get_code(e);
				if (code == LV_EVENT_READY || code == LV_EVENT_DEFOCUSED)
				{
					callback(e);
				}
			},
			LV_EVENT_ALL,
			userData);
	}
} // namespace UI

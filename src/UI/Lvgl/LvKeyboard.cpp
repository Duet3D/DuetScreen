/*
 * LvKeyboard.cpp
 *
 *  Created on: 2025-06-16
 *      Author: Andy Everitt
 */

#include "LvKeyboard.h"
#include "Debug.h"

namespace UI
{
#define LV_KB_BTN(width) static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_POPOVER | width)

	// clang-format off
	static const char * default_kb_map_lc[] = {
		"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
		"1#", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", LV_SYMBOL_BACKSPACE, "\n",
        "ABC", "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_NEW_LINE, "\n",
        "_", "-", "z", "x", "c", "v", "b", "n", "m", ".", ",", ":", "\n",
        LV_SYMBOL_KEYBOARD, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
    };
	
	static const lv_buttonmatrix_ctrl_t default_kb_ctrl_lc_map[] = {
		LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    	static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 7),
    	static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 6), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 7),
    	static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1)), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1)), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1)), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1)), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | LV_KB_BTN(1)),
    	static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 2), LV_KB_BTN(6), static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 2), static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2)
	};
	
	static const char * default_kb_map_uc[] = {
		"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
		"1#", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", LV_SYMBOL_BACKSPACE, "\n",
        "abc", "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_NEW_LINE, "\n",
        "_", "-", "Z", "X", "C", "V", "B", "N", "M", ".", ",", ":", "\n",
        LV_SYMBOL_KEYBOARD, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
    };

	// The number pad: digits, sign and point, then backspace and OK
	static const char* number_pad_map[] = {
		"1", "2", "3", "\n",
		"4", "5", "6", "\n",
		"7", "8", "9", "\n",
		"+/-", "0", ".", "\n",
		LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, ""};

	static const lv_buttonmatrix_ctrl_t number_pad_ctrl_map[] = {
		LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
		LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
		LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_WIDTH_1,
		LV_BUTTONMATRIX_CTRL_CHECKED, LV_BUTTONMATRIX_CTRL_WIDTH_1, LV_BUTTONMATRIX_CTRL_CHECKED,
		LV_BUTTONMATRIX_CTRL_CHECKED, LV_BUTTONMATRIX_CTRL_CHECKED,
	};
	// clang-format on

	LvKeyboard::LvKeyboard(const std::string& name, LvObj& parent)
		: LvKeyboardGen(name, parent)
	{
		UI_LOCK();

		setMap(LV_KEYBOARD_MODE_TEXT_LOWER, default_kb_map_lc, default_kb_ctrl_lc_map);
		setMap(LV_KEYBOARD_MODE_TEXT_UPPER, default_kb_map_uc, default_kb_ctrl_lc_map);
		setMap(NUMBER_PAD_MODE, number_pad_map, number_pad_ctrl_map);
	}

	void LvKeyboard::setTextarea(LvTextarea* ta)
	{
		UI_LOCK();
		LvKeyboardGen::setTextarea(ta ? ta->getRootPtr() : nullptr);
	}

	LvTextarea* LvKeyboard::getTextarea() const
	{
		UI_LOCK();
		return static_cast<LvTextarea*>(LvObj::fromPtr(lv_keyboard_get_textarea(getRootPtr())));
	}
} // namespace UI

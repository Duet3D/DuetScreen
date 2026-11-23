/*
 * LvKeyboard.h
 *
 *  Created on: 2025-06-16
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvTextarea.h"
#include "UI/Lvgl/generated/LvKeyboard.gen.h"

namespace UI
{

	class LvKeyboard : public LvKeyboardGen
	{
	  public:
		/** The number pad layout: digits, +/-, point, backspace and OK */
		static constexpr lv_keyboard_mode_t NUMBER_PAD_MODE = LV_KEYBOARD_MODE_USER_1;

		LvKeyboard(const std::string& name, LvObj& parent);

		void setTextarea(LvTextarea* ta);
		LvTextarea* getTextarea() const;

	  private:
	};
} // namespace UI

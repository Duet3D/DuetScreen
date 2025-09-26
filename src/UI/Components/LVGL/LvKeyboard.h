/*
 * LvKeyboard.h
 *
 *  Created on: 2025-06-16
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include "UI/Components/LVGL/LvTextArea.h"

namespace UI
{

	class LvKeyboard : public LvObj
	{
	  public:
		LvKeyboard(const std::string& name, LvObj& parent);

		void setTextArea(LvTextArea* textArea);
		void setMode(lv_keyboard_mode_t mode);
		void setPopovers(bool enable);
		void setMap(lv_keyboard_mode_t mode, const char* map[], const lv_buttonmatrix_ctrl_t ctrl_map[]);
		lv_obj_t* getTextArea() const;
		lv_keyboard_mode_t getMode() const;
		bool getPopovers() const;
		const char* const* getMapArray() const;
		uint32_t getSelectedButton() const;
		const char* getButtonText(uint32_t index) const;

	  private:
	};
} // namespace UI

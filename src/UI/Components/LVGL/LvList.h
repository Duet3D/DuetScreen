/*
 * LvList.h
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
	class LvList : public LvObj
	{
	  public:
		LvList(const std::string& name, LvObj& parent);

		// Factory helpers
		lv_obj_t* addText(std::string_view txt);
		lv_obj_t* addButton(const void* icon, std::string_view txt);

		// Accessors
		std::string_view getButtonText(lv_obj_t* btn) const;
		void setButtonText(lv_obj_t* btn, std::string_view txt);
	};
} // namespace UI

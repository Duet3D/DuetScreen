/*
 * LvContainer.h
 *
 *  Created on: 2025-06-11
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
	class LvContainer : public LvObj
	{
	  public:
		LvContainer(const std::string& name, LvObj& parent);
		LvContainer(const std::string& name, LvObj& parent, layout_t layout);

	  private:
	};
} // namespace UI

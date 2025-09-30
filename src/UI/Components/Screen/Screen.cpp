/*
 * Screen.cpp
 *
 *  Created on: 2025-09-30
 *      Author: Andy Everitt
 */

#include "Screen.h"
#include "Debug.h"

namespace UI
{
	Screen::Screen(const std::string& name)
		: LvObj(lv_obj_create, name)
	{
		LOG_INFO("Creating Screen: {}", name);
		setSize(LV_PCT(100), LV_PCT(100));
	}
} // namespace UI

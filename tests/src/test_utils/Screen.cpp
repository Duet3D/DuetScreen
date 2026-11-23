/*
 * Screen.cpp
 *
 *  Created on: 2025-09-30
 *      Author: Andy Everitt
 */

#include "Screen.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	Screen::Screen(const std::string& name)
		: LvObj(lv_obj_create, name)
	{
		ZoneScoped;
		LOG_INFO("Creating Screen: {}", name);
		setSize(LV_PCT(100), LV_PCT(100));

		addStyle(Themes::getLvglStyles().text);
	}
} // namespace UI

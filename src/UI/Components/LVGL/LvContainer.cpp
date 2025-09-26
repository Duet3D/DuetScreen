/*
 * LvContainer.cpp
 *
 *  Created on: 2025-06-11
 *      Author: Andy Everitt
 */

#include "LvContainer.h"
#include "Debug.h"

namespace UI
{
	LvContainer::LvContainer(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
	{
	}

	LvContainer::LvContainer(const std::string& name, LvObj& parent, layout_t layout)
		: LvObj(lv_obj_create, name, parent, layout)
	{
	}
} // namespace UI

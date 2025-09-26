/*
 * Column.cpp
 *
 *  Created on: 2025-08-21
 *      Author: Andy Everitt
 */

#include "Column.h"
#include "Debug.h"

namespace UI
{
	Column::Column(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
	}
} // namespace UI

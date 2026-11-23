/*
 * LvTextarea.cpp
 *
 *  Created on: 2025-06-13
 *      Author: Andy Everitt
 */

#include "LvTextarea.h"
#include "Debug.h"

namespace UI
{
	LvTextarea::LvTextarea(const std::string& name, LvObj& parent)
		: LvTextareaGen(name, parent)
	{
		UI_LOCK();

		setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		setCursorClickPos(true);
	}

} // namespace UI

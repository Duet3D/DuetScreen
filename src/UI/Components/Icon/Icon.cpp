/*
 * Icon.cpp
 *
 *  Created on: 2025-08-15
 *      Author: Andy Everitt
 */

#include "Icon.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	Icon::Icon(const std::string& name, LvObj& parent)
		: LvImage(name, parent)
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().icon);
		enableRecolor(true);

		setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
	}

	void Icon::enableRecolor(bool enable)
	{
		UI_LOCK();
		if (enable)
		{
			addStyle(Themes::getLvglStyles().icon_recolor);
		}
		else
		{
			removeStyle(Themes::getLvglStyles().icon_recolor);
		}
	}
} // namespace UI

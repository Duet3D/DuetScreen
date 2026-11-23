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
		ZoneScoped;
		UI_LOCK();

		addStyle(Themes::getLvglStyles().icon);
		enableRecolor(true);

		setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);

		addEventCallback(
			[this](lv_event_t*)
			{
				/* Refresh the icon when the event is triggered */
				if (m_iconName.empty())
					return;
				refreshIcon(m_iconName);
			},
			LV_EVENT_REFRESH);
	}

	void Icon::setIcon(std::string_view icon)
	{
		ZoneScoped;
		UI_LOCK();
		m_iconName = icon;
		refreshIcon(m_iconName);
	}

	void Icon::setFixedIcon(const std::string& icon_path)
	{
		ZoneScoped;
		UI_LOCK();
		m_iconName.clear();
		setSrc(icon_path.empty() ? nullptr : icon_path.c_str());
	}

	void Icon::clearIcon()
	{
		ZoneScoped;
		UI_LOCK();
		m_iconName.clear();
		setSrc(nullptr);
	}

	void Icon::refreshIcon(std::string_view icon)
	{
		ZoneScoped;
		UI_LOCK();
		setSrc(icon.empty() ? nullptr : Themes::getIconPath(icon).c_str());
	}

	void Icon::enableRecolor(bool enable)
	{
		ZoneScoped;
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

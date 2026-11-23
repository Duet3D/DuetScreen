/*
 * Icon.h
 *
 *  Created on: 2025-08-15
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvImage.h"

namespace UI
{
	class Icon : public LvImage
	{
	  public:
		Icon(const std::string& name, LvObj& parent);

		void setIcon(std::string_view icon);
		void setFixedIcon(const std::string& icon_path);
		void clearIcon();
		void enableRecolor(bool enable);
		void setRecolor(lv_color_t color);

		bool isRecolorEnabled() const;
		lv_color_t getRecolor() const;
		std::string_view getIconName() const { return m_iconName; }

	  private:
		void refreshIcon(std::string_view icon);
		std::string m_iconName;
	};
} // namespace UI

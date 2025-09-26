/*
 * Icon.h
 *
 *  Created on: 2025-08-15
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvImage.h"

namespace UI
{
    class Icon : public LvImage
    {
      public:
		Icon(const std::string& name, LvObj& parent);

		void enableRecolor(bool enable);
        void setRecolor(lv_color_t color);

        bool isRecolorEnabled() const;
        lv_color_t getRecolor() const;

      private:
    };
} // namespace UI

/*
 * LvSlider.h
 *
 *  Created on: 2025-07-10
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
    #define LV_SLIDER_DEFAULT_ANIM LV_ANIM_ON

    class LvSlider : public LvObj
    {
      public:
		LvSlider(const std::string& name, LvObj& parent);

		void setValue(int32_t value, lv_anim_enable_t anim = LV_SLIDER_DEFAULT_ANIM);
        void setStartValue(int32_t value, lv_anim_enable_t anim = LV_SLIDER_DEFAULT_ANIM);
        void setRange(int32_t min, int32_t max);
        void setMinValue(int32_t min);
        void setMaxValue(int32_t max);
        void setMode(lv_slider_mode_t mode);
        void setOrientation(lv_slider_orientation_t orientation);

        int32_t getValue() const;
        int32_t getLeftValue() const;
        int32_t getMinValue() const;
        int32_t getMaxValue() const;
        bool isDragged() const;
        lv_slider_mode_t getMode() const;
        lv_slider_orientation_t getOrientation() const;
        bool isSymmetrical() const;
      private:
    };
} // namespace UI

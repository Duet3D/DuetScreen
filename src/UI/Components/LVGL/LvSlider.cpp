/*
 * LvSlider.cpp
 *
 *  Created on: 2025-07-10
 *      Author: Andy Everitt
 */

#include "LvSlider.h"
#include "Debug.h"

namespace UI
{
	LvSlider::LvSlider(const std::string& name, LvObj& parent)
		: LvObj(lv_slider_create, name, parent)
	{
		UI_LOCK();
	}

	void LvSlider::setValue(int32_t value, lv_anim_enable_t anim)
	{
		UI_LOCK();
		lv_slider_set_value(getRoot(), value, anim);
	}

	void LvSlider::setStartValue(int32_t value, lv_anim_enable_t anim)
	{
		UI_LOCK();
		lv_slider_set_start_value(getRoot(), value, anim);
	}

	void LvSlider::setRange(int32_t min, int32_t max)
	{
		UI_LOCK();
		lv_slider_set_range(getRoot(), min, max);
	}

	void LvSlider::setMinValue(int32_t min)
	{
		UI_LOCK();
		lv_slider_set_min_value(getRoot(), min);
	}

	void LvSlider::setMaxValue(int32_t max)
	{
		UI_LOCK();
		lv_slider_set_max_value(getRoot(), max);
	}

	void LvSlider::setMode(lv_slider_mode_t mode)
	{
		UI_LOCK();
		lv_slider_set_mode(getRoot(), mode);
	}

	void LvSlider::setOrientation(lv_slider_orientation_t orientation)
	{
		UI_LOCK();
		lv_slider_set_orientation(getRoot(), orientation);
	}

	int32_t LvSlider::getValue() const
	{
		UI_LOCK();
		return lv_slider_get_value(getRoot());
	}

	int32_t LvSlider::getLeftValue() const
	{
		UI_LOCK();
		return lv_slider_get_left_value(getRoot());
	}

	int32_t LvSlider::getMinValue() const
	{
		UI_LOCK();
		return lv_slider_get_min_value(getRoot());
	}

	int32_t LvSlider::getMaxValue() const
	{
		UI_LOCK();
		return lv_slider_get_max_value(getRoot());
	}

	bool LvSlider::isDragged() const
	{
		UI_LOCK();
		return lv_slider_is_dragged(getRoot());
	}

	lv_slider_mode_t LvSlider::getMode() const
	{
		UI_LOCK();
		return lv_slider_get_mode(getRoot());
	}

	lv_slider_orientation_t LvSlider::getOrientation() const
	{
		UI_LOCK();
		return lv_slider_get_orientation(getRoot());
	}

	bool LvSlider::isSymmetrical() const
	{
		UI_LOCK();
		return lv_slider_is_symmetrical(getRoot());
	}
} // namespace UI

/*
 * LvBar.cpp
 *
 *  Created on: 2025-07-10
 *      Author: Andy Everitt
 */

#include "LvBar.h"
#include "Debug.h"

namespace UI
{
	LvBar::LvBar(const std::string& name, LvObj& parent)
		: LvObj(lv_bar_create, name, parent)
	{
		UI_LOCK();
	}

	void LvBar::setValue(int32_t value, lv_anim_enable_t anim)
	{
		UI_LOCK();
		lv_bar_set_value(getRoot(), value, anim);
	}

	void LvBar::setStartValue(int32_t value, lv_anim_enable_t anim)
	{
		UI_LOCK();
		lv_bar_set_start_value(getRoot(), value, anim);
	}

	void LvBar::setRange(int32_t min, int32_t max)
	{
		UI_LOCK();
		lv_bar_set_range(getRoot(), min, max);
	}

	void LvBar::setMinValue(int32_t min)
	{
		UI_LOCK();
		lv_bar_set_min_value(getRoot(), min);
	}

	void LvBar::setMaxValue(int32_t max)
	{
		UI_LOCK();
		lv_bar_set_max_value(getRoot(), max);
	}

	void LvBar::setMode(lv_bar_mode_t mode)
	{
		UI_LOCK();
		lv_bar_set_mode(getRoot(), mode);
	}

	void LvBar::setOrientation(lv_bar_orientation_t orientation)
	{
		UI_LOCK();
		lv_bar_set_orientation(getRoot(), orientation);
	}

	int32_t LvBar::getValue() const
	{
		UI_LOCK();
		return lv_bar_get_value(getRoot());
	}

	int32_t LvBar::getStartValue() const
	{
		UI_LOCK();
		return lv_bar_get_start_value(getRoot());
	}

	int32_t LvBar::getMinValue() const
	{
		UI_LOCK();
		return lv_bar_get_min_value(getRoot());
	}

	int32_t LvBar::getMaxValue() const
	{
		UI_LOCK();
		return lv_bar_get_max_value(getRoot());
	}

	lv_bar_mode_t LvBar::getMode() const
	{
		UI_LOCK();
		return lv_bar_get_mode(getRoot());
	}

	lv_bar_orientation_t LvBar::getOrientation() const
	{
		UI_LOCK();
		return lv_bar_get_orientation(getRoot());
	}

	bool LvBar::isSymmetrical() const
	{
		UI_LOCK();
		return lv_bar_is_symmetrical(getRoot());
	}
} // namespace UI

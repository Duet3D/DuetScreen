/*
 * LvArc.h
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
	class LvArc : public LvObj
	{
	  public:
		LvArc(const std::string& name, LvObj& parent);

		// Setters
		void setStartAngle(lv_value_precise_t start);
		void setEndAngle(lv_value_precise_t end);
		void setAngles(lv_value_precise_t start, lv_value_precise_t end);
		void setBgStartAngle(lv_value_precise_t start);
		void setBgEndAngle(lv_value_precise_t end);
		void setBgAngles(lv_value_precise_t start, lv_value_precise_t end);
		void setRotation(int32_t rotation);
		void setMode(lv_arc_mode_t mode);
		void setValue(int32_t value);
		void setRange(int32_t min, int32_t max);
		void setMinValue(int32_t min);
		void setMaxValue(int32_t max);
		void setChangeRate(uint32_t rate);
		void setKnobOffset(int32_t offset);

		// Getters
		lv_value_precise_t getStartAngle() const;
		lv_value_precise_t getEndAngle() const;
		lv_value_precise_t getBgStartAngle() const;
		lv_value_precise_t getBgEndAngle() const;
		int32_t getValue() const;
		int32_t getMinValue() const;
		int32_t getMaxValue() const;
		lv_arc_mode_t getMode() const;
		int32_t getRotation() const;
		int32_t getKnobOffset() const;

		// Utilities
		void alignObjToAngle(lv_obj_t* obj_to_align, int32_t r_offset);
		void rotateObjToAngle(lv_obj_t* obj_to_rotate, int32_t r_offset);
	};
} // namespace UI

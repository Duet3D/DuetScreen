/*
 * LvArc.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "LvArc.h"
#include "Debug.h"

namespace UI
{
	/**
	 * Create an arc object
	 * - Wraps lv_arc_create to instantiate an LVGL arc as this object's root.
	 * @param name   Debug/name tag for this wrapper instance
	 * @param parent Pointer to an object, it will be the parent of the new arc
	 */
	LvArc::LvArc(const std::string& name, LvObj& parent)
		: LvObj(lv_arc_create, name, parent)
	{
		UI_LOCK();
	}

	/**
	 * Set the start angle of an arc. 0 deg: right, 90 bottom, etc.
	 * @param start the start angle. (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArc::setStartAngle(lv_value_precise_t start)
	{
		UI_LOCK();
		lv_arc_set_start_angle(getRoot(), start);
	}

	/**
	 * Set the end angle of an arc. 0 deg: right, 90 bottom, etc.
	 * @param end the end angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArc::setEndAngle(lv_value_precise_t end)
	{
		UI_LOCK();
		lv_arc_set_end_angle(getRoot(), end);
	}

	/**
	 * Set the start and end angles.
	 * @param start the start angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 * @param end   the end angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArc::setAngles(lv_value_precise_t start, lv_value_precise_t end)
	{
		UI_LOCK();
		lv_arc_set_angles(getRoot(), start, end);
	}

	/**
	 * Set the start angle of an arc background. 0 deg: right, 90 bottom, etc.
	 * @param start the start angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArc::setBgStartAngle(lv_value_precise_t start)
	{
		UI_LOCK();
		lv_arc_set_bg_start_angle(getRoot(), start);
	}

	/**
	 * Set the end angle of an arc background. 0 deg: right, 90 bottom, etc.
	 * @param end the end angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArc::setBgEndAngle(lv_value_precise_t end)
	{
		UI_LOCK();
		lv_arc_set_bg_end_angle(getRoot(), end);
	}

	/**
	 * Set the start and end angles of the arc background.
	 * @param start the start angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 * @param end   the end angle (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArc::setBgAngles(lv_value_precise_t start, lv_value_precise_t end)
	{
		UI_LOCK();
		lv_arc_set_bg_angles(getRoot(), start, end);
	}

	/**
	 * Set the rotation for the whole arc
	 * @param rotation rotation angle
	 */
	void LvArc::setRotation(int32_t rotation)
	{
		UI_LOCK();
		lv_arc_set_rotation(getRoot(), rotation);
	}

	/**
	 * Set the type (mode) of arc.
	 * @param mode arc's mode
	 */
	void LvArc::setMode(lv_arc_mode_t mode)
	{
		UI_LOCK();
		lv_arc_set_mode(getRoot(), mode);
	}

	/**
	 * Set a new value on the arc
	 * @param value new value
	 */
	void LvArc::setValue(int32_t value)
	{
		UI_LOCK();
		lv_arc_set_value(getRoot(), value);
	}

	/**
	 * Set minimum and the maximum values of an arc
	 * @param min minimum value
	 * @param max maximum value
	 */
	void LvArc::setRange(int32_t min, int32_t max)
	{
		UI_LOCK();
		lv_arc_set_range(getRoot(), min, max);
	}

	/**
	 * Set the minimum value of an arc
	 * @param min minimum value
	 */
	void LvArc::setMinValue(int32_t min)
	{
		UI_LOCK();
		lv_arc_set_min_value(getRoot(), min);
	}

	/**
	 * Set the maximum value of an arc
	 * @param max maximum value
	 */
	void LvArc::setMaxValue(int32_t max)
	{
		UI_LOCK();
		lv_arc_set_max_value(getRoot(), max);
	}

	/**
	 * Set a change rate to limit the speed how fast the arc should reach the pressed point.
	 * @param rate the change rate
	 */
	void LvArc::setChangeRate(uint32_t rate)
	{
		UI_LOCK();
		lv_arc_set_change_rate(getRoot(), rate);
	}

	/**
	 * Set an offset angle for the knob
	 * @param offset knob offset from main arc in degrees
	 */
	void LvArc::setKnobOffset(int32_t offset)
	{
		UI_LOCK();
		lv_arc_set_knob_offset(getRoot(), offset);
	}

	/**
	 * Get the start angle of an arc.
	 * @return the start angle [0..360] (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	lv_value_precise_t LvArc::getStartAngle() const
	{
		UI_LOCK();
		return lv_arc_get_angle_start(getRoot());
	}

	/**
	 * Get the end angle of an arc.
	 * @return the end angle [0..360] (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	lv_value_precise_t LvArc::getEndAngle() const
	{
		UI_LOCK();
		return lv_arc_get_angle_end(getRoot());
	}

	/**
	 * Get the start angle of an arc background.
	 * @return the start angle [0..360] (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	lv_value_precise_t LvArc::getBgStartAngle() const
	{
		UI_LOCK();
		return lv_arc_get_bg_angle_start(getRoot());
	}

	/**
	 * Get the end angle of an arc background.
	 * @return the end angle [0..360] (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	lv_value_precise_t LvArc::getBgEndAngle() const
	{
		UI_LOCK();
		return lv_arc_get_bg_angle_end(getRoot());
	}

	/** Get the value of an arc. */
	int32_t LvArc::getValue() const
	{
		UI_LOCK();
		return lv_arc_get_value(getRoot());
	}

	/** Get the minimum value of an arc. */
	int32_t LvArc::getMinValue() const
	{
		UI_LOCK();
		return lv_arc_get_min_value(getRoot());
	}

	/** Get the maximum value of an arc. */
	int32_t LvArc::getMaxValue() const
	{
		UI_LOCK();
		return lv_arc_get_max_value(getRoot());
	}

	/**
	 * Get whether the arc is type or not.
	 * @return arc's mode
	 */
	lv_arc_mode_t LvArc::getMode() const
	{
		UI_LOCK();
		return lv_arc_get_mode(getRoot());
	}

	/** Get the rotation for the whole arc. */
	int32_t LvArc::getRotation() const
	{
		UI_LOCK();
		return lv_arc_get_rotation(getRoot());
	}

	/** Get the current knob angle offset. */
	int32_t LvArc::getKnobOffset() const
	{
		UI_LOCK();
		return lv_arc_get_knob_offset(getRoot());
	}

	/**
	 * Align an object to the current position of the arc (knob)
	 * @param obj_to_align pointer to an object to align
	 * @param r_offset     consider the radius larger with this value (< 0: for smaller radius)
	 */
	void LvArc::alignObjToAngle(lv_obj_t* obj_to_align, int32_t r_offset)
	{
		UI_LOCK();
		lv_arc_align_obj_to_angle(getRoot(), obj_to_align, r_offset);
	}

	/**
	 * Rotate an object to the current position of the arc (knob)
	 * @param obj_to_rotate pointer to an object to rotate
	 * @param r_offset      consider the radius larger with this value (< 0: for smaller radius)
	 */
	void LvArc::rotateObjToAngle(lv_obj_t* obj_to_rotate, int32_t r_offset)
	{
		UI_LOCK();
		lv_arc_rotate_obj_to_angle(getRoot(), obj_to_rotate, r_offset);
	}
} // namespace UI

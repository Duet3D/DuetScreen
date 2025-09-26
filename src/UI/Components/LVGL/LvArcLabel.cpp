/*
 * LvArcLabel.cpp
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#include "LvArcLabel.h"
#include "Debug.h"
#include <cstdarg>

namespace UI
{
	/**
	 * Create an arc label object
	 * - Wraps lv_arclabel_create to instantiate an LVGL arc label as this object's root.
	 * @param name   Debug/name tag for this wrapper instance
	 * @param parent Pointer to an object, it will be the parent of the new arc label
	 */
	LvArcLabel::LvArcLabel(const std::string& name, LvObj& parent)
		: LvObj(lv_arclabel_create, name, parent)
	{
		UI_LOCK();
	}

	/**
	 * Set the text of the arc label.
	 * This function sets the text displayed by an arc label object.
	 * @param text Pointer to a null-terminated string containing the new text for the label.
	 */
	void LvArcLabel::setText(std::string_view text)
	{
		UI_LOCK();
		lv_arclabel_set_text_fmt(getRoot(), "%.*s", (int)text.length(), text.data());
	}

	/**
	 * Sets a new static text for the arc label or refreshes it with the current text.
	 * The 'text' must remain valid in memory; the arc label does not manage its lifecycle.
	 * @param text Pointer to the new text. If NULL, the label is refreshed with its current text.
	 */
	void LvArcLabel::setTextStatic(std::string_view text)
	{
		UI_LOCK();
		lv_arclabel_set_text_static(getRoot(), text.data());
	}

	/**
	 * Set the start angle of an arc. 0 deg: right, 90 bottom, etc.
	 * @param start the start angle. (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArcLabel::setAngleStart(lv_value_precise_t start)
	{
		UI_LOCK();
		lv_arclabel_set_angle_start(getRoot(), start);
	}

	/**
	 * Set the end angle of an arc. 0 deg: right, 90 bottom, etc.
	 * @param size the angle size (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	void LvArcLabel::setAngleSize(lv_value_precise_t size)
	{
		UI_LOCK();
		lv_arclabel_set_angle_size(getRoot(), size);
	}

	/**
	 * Set the rotation for the whole arc
	 * @param offset rotation angle
	 */
	void LvArcLabel::setOffset(int32_t offset)
	{
		UI_LOCK();
		lv_arclabel_set_offset(getRoot(), offset);
	}

	/**
	 * Set the type of arc.
	 * @param dir arc label's direction
	 */
	void LvArcLabel::setDir(lv_arclabel_dir_t dir)
	{
		UI_LOCK();
		lv_arclabel_set_dir(getRoot(), dir);
	}

	/**
	 * Enable the recoloring by in-line commands
	 * Example: "This is a #ff0000 red# word"
	 * @param enable true: enable recoloring, false: disable
	 */
	void LvArcLabel::setRecolor(bool enable)
	{
		UI_LOCK();
		lv_arclabel_set_recolor(getRoot(), enable);
	}

	/**
	 * Set the radius for an arc label object.
	 * @param radius The radius value to set for the label's curvature, in pixels.
	 */
	void LvArcLabel::setRadius(uint32_t radius)
	{
		UI_LOCK();
		lv_arclabel_set_radius(getRoot(), radius);
	}

	/**
	 * Set the center offset x for an arc label object.
	 * @param x the x offset
	 */
	void LvArcLabel::setCenterOffsetX(uint32_t x)
	{
		UI_LOCK();
		lv_arclabel_set_center_offset_x(getRoot(), x);
	}

	/**
	 * Set the center offset y for an arc label object.
	 * @param y the y offset
	 */
	void LvArcLabel::setCenterOffsetY(uint32_t y)
	{
		UI_LOCK();
		lv_arclabel_set_center_offset_y(getRoot(), y);
	}

	/**
	 * Set the text vertical alignment for an arc label object.
	 * @param align the vertical alignment
	 */
	void LvArcLabel::setTextVerticalAlign(lv_arclabel_text_align_t align)
	{
		UI_LOCK();
		lv_arclabel_set_text_vertical_align(getRoot(), align);
	}

	/**
	 * Set the text horizontal alignment for an arc label object.
	 * @param align the horizontal alignment
	 */
	void LvArcLabel::setTextHorizontalAlign(lv_arclabel_text_align_t align)
	{
		UI_LOCK();
		lv_arclabel_set_text_horizontal_align(getRoot(), align);
	}

	/**
	 * Get the start angle of an arc label.
	 * @return the start angle [0..360] (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	lv_value_precise_t LvArcLabel::getAngleStart() const
	{
		UI_LOCK();
		return lv_arclabel_get_angle_start(getRoot());
	}

	/**
	 * Get the angle size of an arc label.
	 * @return the end angle [0..360] (if LV_USE_FLOAT is enabled it can be fractional too.)
	 */
	lv_value_precise_t LvArcLabel::getAngleSize() const
	{
		UI_LOCK();
		return lv_arclabel_get_angle_size(getRoot());
	}

	/**
	 * Get whether the arc label is type or not.
	 * @return arc label's direction
	 */
	lv_arclabel_dir_t LvArcLabel::getDir() const
	{
		UI_LOCK();
		return lv_arclabel_get_dir(getRoot());
	}

	/**
	 * Enable the recoloring by in-line commands
	 * @return true: enable recoloring, false: disable
	 */
	bool LvArcLabel::getRecolor() const
	{
		UI_LOCK();
		return lv_arclabel_get_recolor(getRoot());
	}

	/** Get the radius of the arc label. */
	uint32_t LvArcLabel::getRadius() const
	{
		UI_LOCK();
		return lv_arclabel_get_radius(getRoot());
	}

	/** Get the center offset x for an arc label object. */
	uint32_t LvArcLabel::getCenterOffsetX() const
	{
		UI_LOCK();
		return lv_arclabel_get_center_offset_x(getRoot());
	}

	/** Get the center offset y for an arc label object. */
	uint32_t LvArcLabel::getCenterOffsetY() const
	{
		UI_LOCK();
		return lv_arclabel_get_center_offset_y(getRoot());
	}

	/** Get the text vertical alignment for an arc label object. */
	lv_arclabel_text_align_t LvArcLabel::getTextVerticalAlign() const
	{
		UI_LOCK();
		return lv_arclabel_get_text_vertical_align(getRoot());
	}

	/** Get the text horizontal alignment for an arc label object. */
	lv_arclabel_text_align_t LvArcLabel::getTextHorizontalAlign() const
	{
		UI_LOCK();
		return lv_arclabel_get_text_horizontal_align(getRoot());
	}
} // namespace UI

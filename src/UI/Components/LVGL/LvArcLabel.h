/*
 * LvArcLabel.h
 *
 *  Created on: 2025-08-19
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"

namespace UI
{
	class LvArcLabel : public LvObj
	{
	  public:
		LvArcLabel(const std::string& name, LvObj& parent);

		// Setters
		void setText(std::string_view text);
		void setTextStatic(std::string_view text);
		void setAngleStart(lv_value_precise_t start);
		void setAngleSize(lv_value_precise_t size);
		void setOffset(int32_t offset);
		void setDir(lv_arclabel_dir_t dir);
		void setRecolor(bool enable);
		void setRadius(uint32_t radius);
		void setCenterOffsetX(uint32_t x);
		void setCenterOffsetY(uint32_t y);
		void setTextVerticalAlign(lv_arclabel_text_align_t align);
		void setTextHorizontalAlign(lv_arclabel_text_align_t align);

		// Getters
		lv_value_precise_t getAngleStart() const;
		lv_value_precise_t getAngleSize() const;
		lv_arclabel_dir_t getDir() const;
		bool getRecolor() const;
		uint32_t getRadius() const;
		uint32_t getCenterOffsetX() const;
		uint32_t getCenterOffsetY() const;
		lv_arclabel_text_align_t getTextVerticalAlign() const;
		lv_arclabel_text_align_t getTextHorizontalAlign() const;
	};
} // namespace UI

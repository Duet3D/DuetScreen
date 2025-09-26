/*
 * LvImage.h
 *
 *  Created on: 2025-08-11
 */

#pragma once

#include "LvObj.h"

#if SIMULATION
#  define IMAGE_ASSET(name) "A:assets/" name
#  define VIDEO_ASSET(name) "assets/" name
#else
#  define IMAGE_ASSET(name) "A:/etc/assets/" name
#  define VIDEO_ASSET(name) "/etc/assets/" name
#endif

namespace UI
{
	class LvImage : public LvObj
	{
	  public:
		LvImage(const std::string& name, LvObj& parent);

		// Setters
		void setSrc(const void* src);
		void setOffsetX(int32_t x);
		void setOffsetY(int32_t y);
		void setRotation(int32_t angle);
		void setPivot(int32_t x, int32_t y);
		void setPivotX(int32_t x);
		void setPivotY(int32_t y);
		void setScale(uint32_t zoom);
		void setScaleX(uint32_t zoom);
		void setScaleY(uint32_t zoom);
		void setBlendMode(lv_blend_mode_t mode);
		void setAntialias(bool enable);
		void setInnerAlign(lv_image_align_t align);
		void setBitmapMapSrc(const lv_image_dsc_t* src);

		// Getters
		const void* getSrc() const;
		int32_t getOffsetX() const;
		int32_t getOffsetY() const;
		int32_t getRotation() const;
		void getPivot(lv_point_t* pivot) const;
		int32_t getScale() const;
		int32_t getScaleX() const;
		int32_t getScaleY() const;
		int32_t getSrcWidth() const;
		int32_t getSrcHeight() const;
		int32_t getTransformedWidth() const;
		int32_t getTransformedHeight() const;
		lv_blend_mode_t getBlendMode() const;
		bool getAntialias() const;
		lv_image_align_t getInnerAlign() const;
		const lv_image_dsc_t* getBitmapMapSrc() const;
	};
} // namespace UI

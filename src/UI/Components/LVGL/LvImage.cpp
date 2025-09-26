#include "LvImage.h"
#include "Debug.h"

namespace UI
{
	LvImage::LvImage(const std::string& name, LvObj& parent)
		: LvObj(lv_image_create, name, parent)
	{
		setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
	}

	void LvImage::setSrc(const void* src)
	{
		UI_LOCK();
		lv_image_set_src(getRoot(), src);
	}
	void LvImage::setOffsetX(int32_t x)
	{
		UI_LOCK();
		lv_image_set_offset_x(getRoot(), x);
	}
	void LvImage::setOffsetY(int32_t y)
	{
		UI_LOCK();
		lv_image_set_offset_y(getRoot(), y);
	}
	void LvImage::setRotation(int32_t angle)
	{
		UI_LOCK();
		lv_image_set_rotation(getRoot(), angle);
	}
	void LvImage::setPivot(int32_t x, int32_t y)
	{
		UI_LOCK();
		lv_image_set_pivot(getRoot(), x, y);
	}
	void LvImage::setPivotX(int32_t x)
	{
		UI_LOCK();
		lv_image_set_pivot_x(getRoot(), x);
	}
	void LvImage::setPivotY(int32_t y)
	{
		UI_LOCK();
		lv_image_set_pivot_y(getRoot(), y);
	}
	void LvImage::setScale(uint32_t zoom)
	{
		UI_LOCK();
		lv_image_set_scale(getRoot(), zoom);
	}
	void LvImage::setScaleX(uint32_t zoom)
	{
		UI_LOCK();
		lv_image_set_scale_x(getRoot(), zoom);
	}
	void LvImage::setScaleY(uint32_t zoom)
	{
		UI_LOCK();
		lv_image_set_scale_y(getRoot(), zoom);
	}
	void LvImage::setBlendMode(lv_blend_mode_t mode)
	{
		UI_LOCK();
		lv_image_set_blend_mode(getRoot(), mode);
	}
	void LvImage::setAntialias(bool enable)
	{
		UI_LOCK();
		lv_image_set_antialias(getRoot(), enable);
	}
	void LvImage::setInnerAlign(lv_image_align_t align)
	{
		UI_LOCK();
		lv_image_set_inner_align(getRoot(), align);
	}
	void LvImage::setBitmapMapSrc(const lv_image_dsc_t* src)
	{
		UI_LOCK();
		lv_image_set_bitmap_map_src(getRoot(), src);
	}

	const void* LvImage::getSrc() const
	{
		UI_LOCK();
		return lv_image_get_src(getRoot());
	}
	int32_t LvImage::getOffsetX() const
	{
		UI_LOCK();
		return lv_image_get_offset_x(getRoot());
	}
	int32_t LvImage::getOffsetY() const
	{
		UI_LOCK();
		return lv_image_get_offset_y(getRoot());
	}
	int32_t LvImage::getRotation() const
	{
		UI_LOCK();
		return lv_image_get_rotation(getRoot());
	}
	void LvImage::getPivot(lv_point_t* pivot) const
	{
		UI_LOCK();
		lv_image_get_pivot(getRoot(), pivot);
	}
	int32_t LvImage::getScale() const
	{
		UI_LOCK();
		return lv_image_get_scale(getRoot());
	}
	int32_t LvImage::getScaleX() const
	{
		UI_LOCK();
		return lv_image_get_scale_x(getRoot());
	}
	int32_t LvImage::getScaleY() const
	{
		UI_LOCK();
		return lv_image_get_scale_y(getRoot());
	}
	int32_t LvImage::getSrcWidth() const
	{
		UI_LOCK();
		return lv_image_get_src_width(getRoot());
	}
	int32_t LvImage::getSrcHeight() const
	{
		UI_LOCK();
		return lv_image_get_src_height(getRoot());
	}
	int32_t LvImage::getTransformedWidth() const
	{
		UI_LOCK();
		return lv_image_get_transformed_width(getRoot());
	}
	int32_t LvImage::getTransformedHeight() const
	{
		UI_LOCK();
		return lv_image_get_transformed_height(getRoot());
	}
	lv_blend_mode_t LvImage::getBlendMode() const
	{
		UI_LOCK();
		return lv_image_get_blend_mode(getRoot());
	}
	bool LvImage::getAntialias() const
	{
		UI_LOCK();
		return lv_image_get_antialias(getRoot());
	}
	lv_image_align_t LvImage::getInnerAlign() const
	{
		UI_LOCK();
		return lv_image_get_inner_align(getRoot());
	}
	const lv_image_dsc_t* LvImage::getBitmapMapSrc() const
	{
		UI_LOCK();
		return lv_image_get_bitmap_map_src(getRoot());
	}
} // namespace UI

#pragma once

#include "UI/Lvgl/generated/LvCanvas.gen.h"

namespace UI
{
	class LvCanvas : public LvCanvasGen
	{
	  public:
		using LvCanvasGen::LvCanvasGen;

		void setPxSkipInvalidation(int32_t x, int32_t y, lv_color_t color, lv_opa_t opa)
		{
			ZoneScoped;
			UI_LOCK();
			lv_display_enable_invalidation(lv_display_get_default(), false);
			setPx(x, y, color, opa);
			lv_display_enable_invalidation(lv_display_get_default(), true);
		}
	};
} // namespace UI

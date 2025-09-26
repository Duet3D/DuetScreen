/*
 * Canvas.h
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Core/View.h"
#include <map>
#include <memory>

namespace UI
{

	class Canvas : public LvObj
	{
	  public:
		struct range_t
		{
			int32_t min;
			int32_t max;
		};
		struct range_float_t
		{
			float min;
			float max;
		};

		Canvas(const std::string& name, LvObj& parent);
		Canvas(const std::string& name, LvObj& parent, layout_t layout);
		virtual ~Canvas();

		range_t getXRange() const;
		range_t getYRange() const;
		void setXRange(range_t range);
		void setYRange(range_t range);
		void setXRange(range_float_t range);
		void setYRange(range_float_t range);

		bool pxToPos(size_t px, size_t py, float& x, float& y) const;
		bool pxToPos(lv_point_t p, float& x, float& y) const;
		bool posToPx(float x, float y, size_t& px, size_t& py) const;
		bool posToPx(float x, float y, int32_t& px, int32_t& py) const;
		bool posToPx(float x, float y, lv_point_t& p) const;

		void setTitle(std::string_view title);

		void showTitle(const bool show);
		void showXScale(const bool show);
		void showYScale(const bool show);

		bool getResolution(uint32_t& width, uint32_t& height) const;
		void setResolution(uint32_t width, uint32_t height);

		void drawGrid();
		void drawPx(size_t px, size_t py, lv_color_t color, lv_opa_t opa);
		void drawRect(lv_area_t area, int32_t radius, lv_color_t color, lv_opa_t opa);
		void drawRectPx(lv_area_t area, int32_t radius, lv_color_t color, lv_opa_t opa);
		void drawLine(lv_point_t p1, lv_point_t p2, lv_color_t color, lv_opa_t opa);
		void drawLinePx(lv_point_t p1, lv_point_t p2, lv_color_t color, lv_opa_t opa);
		void drawCircle(lv_point_t center, uint32_t radius, lv_color_t color, lv_opa_t opa);
		void drawCirclePx(lv_point_t center, uint32_t radius, lv_color_t color, lv_opa_t opa);
		void drawLabelPx(lv_point_t pos, const std::string& label, lv_color_t color, lv_opa_t opa);

		lv_color_t getPx(size_t px, size_t py) const;

		lv_obj_t* getCanvasObj() const { return m_canvas; }

		void clear();

	  private:
		void init();

		void createLabels(range_float_t range, uint32_t ticks, std::vector<std::string>& vec, const char**& labels);

		int32_t m_columnDsc[3];
		int32_t m_rowDsc[4];

		lv_draw_buf_t* m_buf = nullptr;

		LvLabel m_title;
		lv_obj_t* m_canvas;
		lv_obj_t* m_vScale;
		lv_obj_t* m_hScale;

		std::vector<std::string> m_xLabels;
		std::vector<std::string> m_yLabels;
		const char** m_xLabelPtr = nullptr;
		const char** m_yLabelPtr = nullptr;
	};
} // namespace UI

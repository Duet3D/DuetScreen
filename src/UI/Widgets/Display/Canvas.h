/*
 * Canvas.h
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Widgets/Buttons/Button.h"
#include "UI/Lvgl/LvCanvas.h"
#include "UI/Lvgl/LvScale.h"
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

		LvContainer& getHeader() { return m_header; }
		LvLabel& getTitle() { return m_title; }
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

		LvCanvas& getCanvas() { return m_canvas; }
		lv_draw_buf_t* getDrawBuf() const { return m_canvas.getDrawBuf(); }

		void clear();

	  private:
		void init();

		void createLabels(range_float_t range, uint32_t ticks, std::vector<const char*>& labels);

		int32_t m_columnDsc[3];
		int32_t m_rowDsc[4];

		lv_draw_buf_t* m_buf = nullptr;

		LvContainer m_header{"header", getRoot()};
		LvLabel m_title{"title", m_header};
		LvCanvas m_canvas{"canvas", getRoot()};
		LvScale m_vScale{"vscale", getRoot()};
		LvScale m_hScale{"hscale", getRoot()};

		std::vector<const char*> m_xLabelPtr;
		std::vector<const char*> m_yLabelPtr;
	};
} // namespace UI

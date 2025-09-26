/*
 * Heatmap.h
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Canvas/Canvas.h"
#include <map>
#include <memory>
#include <vector>

namespace UI
{
	class Heatmap : public LvObj
	{
	  public:
		using range_t = Canvas::range_t;
		using range_float_t = Canvas::range_float_t;

		struct DataPoint
		{
			float x;
			float y;
			float value;
		};

		Heatmap(const std::string& name, LvObj& parent);
		Heatmap(const std::string& name, LvObj& parent, layout_t layout);
		virtual ~Heatmap();

		range_t getXRange() const;
		range_t getYRange() const;
		void setXRange(range_t range) { m_canvas.setXRange(range); }
		void setYRange(range_t range) { m_canvas.setYRange(range); }

		void setTitle(std::string_view title);
		void showScale(const bool show);

		void setValueRange(float min, float max);

		bool getResolution(uint32_t& width, uint32_t& height) const { return m_canvas.getResolution(width, height); }
		void setResolution(uint32_t width, uint32_t height) { m_canvas.setResolution(width, height); }

		bool pxToPos(size_t px, size_t py, float& x, float& y) const { return m_canvas.pxToPos(px, py, x, y); }
		bool posToPx(float x, float y, size_t& px, size_t& py) const { return m_canvas.posToPx(x, y, px, py); }

		void setPx(size_t px, size_t py, float value);
		void setPos(float x, float y, float value);

		void drawGrid() { m_canvas.drawGrid(); }
		void drawLabel(int32_t x, int32_t y, const std::string& label, lv_color_t color, lv_opa_t opa)
		{
			m_canvas.drawLabelPx({x, y}, label, color, opa);
		}
		void render();
		void renderColorBar();

		Canvas& getCanvas() { return m_canvas; }

		void clear();

	  private:
		void init();
		float normalizeValue(float value) const;

		int32_t m_columnDsc[4];
		int32_t m_rowDsc[3];

		Canvas m_canvas;
		Canvas m_colorBar;

		float m_minValue = -1.0f;
		float m_maxValue = 1.0f;
	};
} // namespace UI

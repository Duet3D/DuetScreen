/*
 * Heatmap.h
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Widgets/Display/Canvas.h"
#include <map>
#include <memory>
#include <vector>

namespace UI
{
	/**
	 * A map of values over x and y, each pixel coloured by its value, with a colour scale beside it. The colours are
	 * the theme's choice (its `data_colours` const): blue to red, or (`grey`) dark to light grey. They follow theme
	 * changes, the pixels already drawn too.
	 */
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
		void setResolution(uint32_t width, uint32_t height)
		{
			m_canvas.setResolution(width, height);
			m_levels.clear();
		}

		bool pxToPos(size_t px, size_t py, float& x, float& y) const { return m_canvas.pxToPos(px, py, x, y); }
		bool posToPx(float x, float y, size_t& px, size_t& py) const { return m_canvas.posToPx(x, y, px, py); }

		/** Colour pixel (px, py) for `value`; values outside the value range take its end colours */
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
		/** Where a pixel hasn't been drawn, in m_levels */
		static constexpr uint8_t NO_LEVEL = 255;

		void init();
		float normalizeValue(float value) const;

		/** The colour for `percent` (0 to 1) of the value range, as the theme colours data */
		lv_color_t colorForPercent(double percent) const;

		/** Read how the theme colours data, then colour the scale and every pixel drawn again */
		void applyTheme();

		/** Forget the levels of the pixels drawn, as the canvas is cleared */
		void clearLevels() { m_levels.clear(); }

		Canvas m_canvas{"canvas", getRoot()};
		Canvas m_colorBar{"color_bar", getRoot()};

		float m_minValue = -1.0f;
		float m_maxValue = 1.0f;

		bool m_grey = false;			// The theme colours data in greys (its data_colours is "grey")
		std::vector<uint8_t> m_levels;	// Each canvas pixel's level, 0 to 254 of the value range, row by row;
										// NO_LEVEL where none was drawn. Empty until a pixel is drawn.
		size_t m_themeListener = 0;		// Recolours the map when the theme changes
	};
} // namespace UI

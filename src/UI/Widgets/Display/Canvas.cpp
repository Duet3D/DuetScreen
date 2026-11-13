/*
 * Canvas.cpp
 *
 *  Created on: 2025-03-24
 *      Author: Andy Everitt
 */

#include "Canvas.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
#define DEFAULT_CANVAS_WIDTH 100
#define DEFAULT_CANVAS_HEIGHT 100

	Canvas::Canvas(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_columnDsc{LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}
		, m_rowDsc{LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}
	{
		ZoneScoped;
		init();
	}

	Canvas::~Canvas()
	{
		ZoneScoped;
		UI_LOCK();
		if (!lv_is_initialized() || m_buf == nullptr)
		{
			return;
		}
		lv_draw_buf_destroy(m_buf);
	}

	void Canvas::init()
	{
		ZoneScoped;
		UI_LOCK();

		// Layout
		setGridDsc(m_columnDsc, m_rowDsc);
		setGridCell(m_header, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_vScale, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		setGridCell(m_hScale, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 2, 1);
		setGridCell(m_canvas, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

		// Header
		m_header.setStylePad(0);
		m_header.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
		m_header.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		m_title.setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		// Horizontal scale
		m_hScale.setHeight(LV_SIZE_CONTENT);
		m_hScale.setMode(LV_SCALE_MODE_HORIZONTAL_BOTTOM);
		m_hScale.setLabelShow(true);
		m_hScale.setTotalTickCount(17);
		m_hScale.setMajorTickEvery(4);
		setExtDrawSize(100); // To prevent clipping of labels

		// Vertical scale
		m_vScale.setWidth(LV_SIZE_CONTENT);
		m_vScale.setMode(LV_SCALE_MODE_VERTICAL_LEFT);
		m_vScale.setLabelShow(true);
		m_vScale.setTotalTickCount(17);
		m_vScale.setMajorTickEvery(4);

		// Canvas
		m_canvas.setInnerAlign(LV_IMAGE_ALIGN_STRETCH);
	}

	void Canvas::setTitle(std::string_view title)
	{
		ZoneScoped;
		m_title.setText(title);
	}

	void Canvas::showTitle(const bool show)
	{
		ZoneScoped;
		UI_LOCK();
		m_title.setVisible(show);
	}

	void Canvas::showXScale(const bool show)
	{
		ZoneScoped;
		UI_LOCK();
		m_hScale.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
	}

	void Canvas::showYScale(const bool show)
	{
		ZoneScoped;
		UI_LOCK();
		m_vScale.setFlag(LV_OBJ_FLAG_HIDDEN, !show);
	}

	Canvas::range_t Canvas::getXRange() const
	{
		ZoneScoped;
		UI_LOCK();
		range_t range;
		range.min = m_hScale.getRangeMinValue();
		range.max = m_hScale.getRangeMaxValue();
		return range;
	}

	Canvas::range_t Canvas::getYRange() const
	{
		ZoneScoped;
		UI_LOCK();
		range_t range;
		range.min = m_vScale.getRangeMinValue();
		range.max = m_vScale.getRangeMaxValue();
		return range;
	}

	void Canvas::setXRange(Canvas::range_t range)
	{
		ZoneScoped;
		UI_LOCK();
		m_xLabelPtr.clear();
		m_hScale.setTextSrc(m_xLabelPtr);
		m_hScale.setRange(range.min, range.max);
	}

	void Canvas::setYRange(Canvas::range_t range)
	{
		ZoneScoped;
		UI_LOCK();
		m_yLabelPtr.clear();
		m_vScale.setTextSrc(m_yLabelPtr);
		m_vScale.setRange(range.min, range.max);
	}

	void Canvas::createLabels(Canvas::range_float_t range, uint32_t ticks, std::vector<const char*>& labels)
	{
		ZoneScoped;
		UI_LOCK();

		for (const char* label : labels)
		{
			delete[] label;
		}

		labels.clear();
		labels.reserve(ticks + 1);
		float step = (range.max - range.min) / (float)(ticks - 1);
		for (uint32_t i = 0; i < ticks; ++i)
		{
			std::string label_txt = fmt::format("{:g}", range.min + step * (float)i);
			char* label_cstr = new char[label_txt.size() + 1];
			std::strncpy(label_cstr, label_txt.c_str(), label_txt.size() + 1);
			labels.push_back(label_cstr);
		}
		labels.push_back(nullptr);
	}

	void Canvas::setXRange(Canvas::range_float_t range)
	{
		ZoneScoped;
		UI_LOCK();
		uint32_t ticks = 1 + m_hScale.getTotalTickCount() / m_hScale.getMajorTickEvery();
		createLabels(range, ticks, m_xLabelPtr);
		m_hScale.setRange(static_cast<int32_t>(range.min), static_cast<int32_t>(range.max));
		m_hScale.setTextSrc(m_xLabelPtr);
	}

	void Canvas::setYRange(Canvas::range_float_t range)
	{
		ZoneScoped;
		UI_LOCK();
		uint32_t ticks = 1 + m_vScale.getTotalTickCount() / m_vScale.getMajorTickEvery();
		createLabels(range, ticks, m_yLabelPtr);
		m_vScale.setRange(static_cast<int32_t>(range.min), static_cast<int32_t>(range.max));
		m_vScale.setTextSrc(m_yLabelPtr);
	}

	bool Canvas::pxToPos(size_t px, size_t py, float& x, float& y) const
	{
		ZoneScoped;
		UI_LOCK();
		uint32_t res_x, res_y;
		getResolution(res_x, res_y);

		if (px >= res_x || py >= res_y)
		{
			return false;
		}

		range_t xRange = getXRange();
		range_t yRange = getYRange();

		x = static_cast<float>(xRange.min) +
			static_cast<float>(px * (xRange.max - xRange.min)) / static_cast<float>(res_x);
		y = static_cast<float>(yRange.min) +
			static_cast<float>(py * (yRange.max - yRange.min)) / static_cast<float>(res_y);

		return true;
	}

	bool Canvas::pxToPos(lv_point_t p, float& x, float& y) const
	{
		ZoneScoped;
		UI_LOCK();
		return pxToPos(p.x, p.y, x, y);
	}

	bool Canvas::posToPx(float x, float y, size_t& px, size_t& py) const
	{
		ZoneScoped;
		UI_LOCK();
		range_t xRange = getXRange();
		range_t yRange = getYRange();

		if (x < static_cast<float>(xRange.min) || x > static_cast<float>(xRange.max) ||
			y < static_cast<float>(yRange.min) || y > static_cast<float>(yRange.max))
		{
			return false;
		}

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);

		px = (size_t)((x - static_cast<float>(xRange.min)) * static_cast<float>(res_x) /
					  static_cast<float>(xRange.max - xRange.min));
		py = (size_t)(res_y - static_cast<uint32_t>((y - static_cast<float>(yRange.min)) * static_cast<float>(res_y) /
													static_cast<float>(yRange.max - yRange.min)));

		return true;
	}

	bool Canvas::posToPx(float x, float y, int32_t& px, int32_t& py) const
	{
		ZoneScoped;
		UI_LOCK();
		size_t pxSize, pySize;
		if (!posToPx(x, y, pxSize, pySize))
		{
			return false;
		}
		px = (int32_t)pxSize;
		py = (int32_t)pySize;
		return true;
	}

	bool Canvas::posToPx(float x, float y, lv_point_t& p) const
	{
		ZoneScoped;
		UI_LOCK();
		size_t pxSize, pySize;
		if (!posToPx(x, y, pxSize, pySize))
		{
			return false;
		}
		p.x = (int32_t)pxSize;
		p.y = (int32_t)pySize;
		return true;
	}

	bool Canvas::getResolution(uint32_t& width, uint32_t& height) const
	{
		ZoneScoped;
		UI_LOCK();
		if (m_buf == nullptr)
		{
			width = 0u;
			height = 0u;
			return false;
		}

		width = m_buf->header.w;
		height = m_buf->header.h;
		return true;
	}

	void Canvas::setResolution(uint32_t width, uint32_t height)
	{
		ZoneScoped;
		UI_LOCK();
		if (m_buf != nullptr)
		{
			lv_draw_buf_destroy(m_buf);
		}
		width = std::max(width, 1u);
		height = std::max(height, 1u);
		m_buf = lv_draw_buf_create(width, height, lv_display_get_color_format(getDisplayPtr()), 0);
		m_canvas.setDrawBuf(m_buf);
		clear();
	}

	void Canvas::drawGrid()
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("Drawing grid");

		uint32_t res_x, res_y;
		if (!getResolution(res_x, res_y))
		{
			LOG_WARN("Canvas resolution not set");
			return;
		}

		constexpr size_t lines = 5;

		for (size_t i = 0; i < lines; i++)
		{
			drawLinePx({(int32_t)(i * (res_x / (lines - 1))), 0},
					   {(int32_t)(i * (res_x / (lines - 1))), (int32_t)res_y},
					   lv_palette_main(LV_PALETTE_GREY),
					   LV_OPA_30);
			drawLinePx({0, (int32_t)(i * (res_y / (lines - 1)))},
					   {(int32_t)res_x, (int32_t)(i * (res_y / (lines - 1)))},
					   lv_palette_main(LV_PALETTE_GREY),
					   LV_OPA_30);
		}
	}

	void Canvas::drawPx(size_t px, size_t py, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		// Draw the pixel
		m_canvas.setPxSkipInvalidation(static_cast<int32_t>(px), static_cast<int32_t>(py), color, opa);
	}

	void Canvas::drawRect(lv_area_t area, int32_t radius, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("area: ({:d}, {:d}), ({:d}, {:d}), radius: {:d}", area.x1, area.y1, area.x2, area.y2, radius);

		if (!posToPx((float)area.x1, (float)area.y1, area.x1, area.y1))
		{
			LOG_WARN("invalid point ({:d}, {:d})", area.x1, area.y1);
			return;
		}
		if (!posToPx((float)area.x2, (float)area.y2, area.x2, area.y2))
		{
			LOG_WARN("invalid point ({:d}, {:d})", area.x2, area.y2);
			return;
		}

		drawRectPx(area, radius, color, opa);
	}

	void Canvas::drawRectPx(lv_area_t area, int32_t radius, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("area: ({:d}, {:d}), ({:d}, {:d}), radius: {:d}", area.x1, area.y1, area.x2, area.y2, radius);

		lv_draw_rect_dsc_t dsc;
		lv_draw_rect_dsc_init(&dsc);
		dsc.bg_opa = opa;
		dsc.bg_color = color;
		dsc.radius = radius;

		lv_layer_t layer;
		m_canvas.initLayer(&layer);

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);
		area.x1 = std::max(area.x1, 0);
		area.y1 = std::max(area.y1, 0);
		area.x2 = std::min(area.x2, (int32_t)res_x - 1);
		area.y2 = std::min(area.y2, (int32_t)res_y - 1);

		lv_draw_rect(&layer, &dsc, &area);

		lv_draw_rect(&layer, &dsc, &area);

		m_canvas.finishLayer(&layer);
	}

	void Canvas::drawLine(lv_point_t p1, lv_point_t p2, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("p1: ({}, {}), p2: ({}, {})", p1.x, p1.y, p2.x, p2.y);

		if (!posToPx(static_cast<float>(p1.x), static_cast<float>(p1.y), p1.x, p1.y))
		{
			LOG_WARN("invalid point ({}, {})", p1.x, p1.y);
			return;
		}
		if (!posToPx(static_cast<float>(p2.x), static_cast<float>(p2.y), p2.x, p2.y))
		{
			LOG_WARN("invalid point ({}, {})", p2.x, p2.y);
			return;
		}
		drawLinePx(p1, p2, color, opa);
	}

	void Canvas::drawLinePx(lv_point_t p1, lv_point_t p2, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("p1: ({}, {}), p2: ({}, {})", p1.x, p1.y, p2.x, p2.y);

		lv_draw_line_dsc_t line_dsc;
		lv_draw_line_dsc_init(&line_dsc);

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);
		p1.x = std::clamp<int32_t>(p1.x, 0, res_x - 1);
		p1.y = std::clamp<int32_t>(p1.y, 0, res_y - 1);
		p2.x = std::clamp<int32_t>(p2.x, 0, res_x - 1);
		p2.y = std::clamp<int32_t>(p2.y, 0, res_y - 1);

		lv_point_precise_t p1_precise = {static_cast<lv_value_precise_t>(p1.x), static_cast<lv_value_precise_t>(p1.y)};
		lv_point_precise_t p2_precise = {static_cast<lv_value_precise_t>(p2.x), static_cast<lv_value_precise_t>(p2.y)};

		line_dsc.p1 = p1_precise;
		line_dsc.p2 = p2_precise;
		line_dsc.width = 1;
		line_dsc.opa = opa;
		line_dsc.color = color;

		lv_layer_t layer;
		m_canvas.initLayer(&layer);

		lv_draw_line(&layer, &line_dsc);

		m_canvas.finishLayer(&layer);
	}

	void Canvas::drawCircle(lv_point_t center, uint32_t radius, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("center: ({:d}, {:d}), radius: {:d}", center.x, center.y, radius);
		if (!posToPx(static_cast<float>(center.x), static_cast<float>(center.y), center.x, center.y))
		{
			LOG_WARN("invalid point ({:d}, {:d})", center.x, center.y);
			return;
		}
		drawCirclePx(center, radius, color, opa);
	}

	void Canvas::drawCirclePx(lv_point_t center, uint32_t radius, lv_color_t color, lv_opa_t opa)
	{
		UI_LOCK();
		LOG_DBG("center: ({:d}, {:d}), radius: {:d}", center.x, center.y, radius);

		lv_area_t area;
		area.x1 = center.x - radius;
		area.y1 = center.y - radius;
		area.x2 = center.x + radius;
		area.y2 = center.y + radius;

		drawRectPx(area, LV_RADIUS_CIRCLE, color, opa);
	}

	void Canvas::drawLabelPx(lv_point_t pos, const std::string& label, lv_color_t color, lv_opa_t opa)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_DBG("pos: ({:d}, {:d}), label: {:s}", pos.x, pos.y, label.c_str());

		lv_draw_label_dsc_t dsc;
		lv_draw_label_dsc_init(&dsc);
		dsc.color = color;
		dsc.opa = opa;
		dsc.font = &lv_font_montserrat_14;
		dsc.text = label.c_str();
		dsc.align = LV_TEXT_ALIGN_CENTER;

		lv_layer_t layer;
		m_canvas.initLayer(&layer);

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);

		lv_point_t txt_size;
		int32_t max_width = std::min((res_x - pos.x) * 2, res_x);
		lv_text_get_size(&txt_size, dsc.text, dsc.font, dsc.letter_space, dsc.line_space, max_width, dsc.flag);

		lv_area_t area;
		area.x1 = pos.x - txt_size.x / 2;
		area.y1 = res_y - (pos.y + txt_size.y / 2);
		area.x2 = pos.x + txt_size.x / 2;
		area.y2 = res_y - (pos.y - txt_size.y / 2);

		// if (area.x1 < 0 || area.y1 < 0 || area.x2 >= (int32_t)res_x || area.y2 >= (int32_t)res_y)
		// {
		// 	LOG_WARN("Label position out of bounds: ({:d}, {:d})", pos.x, pos.y);
		// 	return;
		// }

		lv_draw_label(&layer, &dsc, &area);

		m_canvas.finishLayer(&layer);
	}

	lv_color_t Canvas::getPx(size_t px, size_t py) const
	{
		ZoneScoped;
		lv_color32_t color32 =
			m_canvas.getPx(static_cast<int32_t>(px), static_cast<int32_t>(py)); // not sure why the API wants int32_t
		lv_color_t color = {color32.blue, color32.green, color32.red};
		return color;
	}

	void Canvas::clear()
	{
		ZoneScoped;
		UI_LOCK();
		lv_style_value_t bg_color;
		lv_style_get_prop(Themes::getLvglStyles().canvas, LV_STYLE_BG_COLOR, &bg_color);
		m_canvas.fillBg(bg_color.color, LV_OPA_TRANSP);
	}
} // namespace UI

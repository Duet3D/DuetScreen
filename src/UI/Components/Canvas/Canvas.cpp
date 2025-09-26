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

	static constexpr lv_coord_t s_scaleSize = 30;

	Canvas::Canvas(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_columnDsc{s_scaleSize, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}
		, m_rowDsc{LV_GRID_CONTENT, LV_GRID_FR(1), s_scaleSize, LV_GRID_TEMPLATE_LAST}
		, m_title("title", getRoot())
		, m_canvas(lv_canvas_create(getRoot()))
		, m_vScale(lv_scale_create(getRoot()))
		, m_hScale(lv_scale_create(getRoot()))
	{
		init();
	}

	Canvas::Canvas(const std::string& name, LvObj& parent, layout_t layout)
		: LvObj(lv_obj_create, name, parent, layout)
		, m_columnDsc{s_scaleSize, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}
		, m_rowDsc{LV_GRID_CONTENT, LV_GRID_FR(1), s_scaleSize, LV_GRID_TEMPLATE_LAST}
		, m_title("title", getRoot())
		, m_canvas(lv_canvas_create(getRoot()))
		, m_vScale(lv_scale_create(getRoot()))
		, m_hScale(lv_scale_create(getRoot()))
	{
		init();
	}

	Canvas::~Canvas()
	{
		UI_LOCK();
		if (!lv_is_initialized() || m_buf == nullptr)
		{
			return;
		}
		lv_draw_buf_destroy(m_buf);
	}

	void Canvas::init()
	{
		UI_LOCK();

		// Layout
		lv_obj_set_layout(getRoot(), LV_LAYOUT_GRID);
		lv_obj_set_grid_dsc_array(getRoot(), m_columnDsc, m_rowDsc);
		lv_obj_set_grid_cell(m_title, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_vScale, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		lv_obj_set_grid_cell(m_hScale, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
		lv_obj_set_grid_cell(m_canvas, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

		lv_obj_set_style_pad_top(getRoot(), 10, LV_PART_MAIN);
		lv_obj_set_style_pad_right(getRoot(), 20, LV_PART_MAIN);
		lv_obj_set_style_pad_left(getRoot(), 10, LV_PART_MAIN);
		lv_obj_set_style_pad_bottom(getRoot(), 5, LV_PART_MAIN);

		// Title
		lv_obj_set_size(m_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

		// Horizontal scale
		lv_obj_set_height(m_hScale, LV_SIZE_CONTENT);
		lv_scale_set_mode(m_hScale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
		lv_scale_set_label_show(m_hScale, true);
		lv_scale_set_total_tick_count(m_hScale, 17);
		lv_scale_set_major_tick_every(m_hScale, 4);

		// Vertical scale
		lv_obj_set_width(m_vScale, LV_SIZE_CONTENT);
		lv_scale_set_mode(m_vScale, LV_SCALE_MODE_VERTICAL_LEFT);
		lv_scale_set_label_show(m_vScale, true);
		lv_scale_set_total_tick_count(m_vScale, 17);
		lv_scale_set_major_tick_every(m_vScale, 4);

		// Canvas
		lv_image_set_inner_align(m_canvas, LV_IMAGE_ALIGN_STRETCH);
	}

	void Canvas::setTitle(std::string_view title)
	{
		m_title.setText(title);
	}

	void Canvas::showTitle(const bool show)
	{
		UI_LOCK();
		lv_obj_set_flag(m_title, LV_OBJ_FLAG_HIDDEN, !show);
		m_rowDsc[0] = show ? LV_GRID_CONTENT : 0;
	}

	void Canvas::showXScale(const bool show)
	{
		UI_LOCK();
		lv_obj_set_flag(m_hScale, LV_OBJ_FLAG_HIDDEN, !show);
		m_rowDsc[2] = show ? s_scaleSize : 0;
	}

	void Canvas::showYScale(const bool show)
	{
		UI_LOCK();
		lv_obj_set_flag(m_vScale, LV_OBJ_FLAG_HIDDEN, !show);
		m_columnDsc[0] = show ? s_scaleSize : 0;
	}

	Canvas::range_t Canvas::getXRange() const
	{
		UI_LOCK();
		range_t range;
		range.min = lv_scale_get_range_min_value(m_hScale);
		range.max = lv_scale_get_range_max_value(m_hScale);
		return range;
	}

	Canvas::range_t Canvas::getYRange() const
	{
		UI_LOCK();
		range_t range;
		range.min = lv_scale_get_range_min_value(m_vScale);
		range.max = lv_scale_get_range_max_value(m_vScale);
		return range;
	}

	void Canvas::setXRange(Canvas::range_t range)
	{
		UI_LOCK();
		m_xLabels.clear();
		if (m_xLabelPtr != nullptr)
		{
			delete[] m_xLabelPtr;
			m_xLabelPtr = nullptr;
		}
		lv_scale_set_text_src(m_hScale, nullptr);
		lv_scale_set_range(m_hScale, range.min, range.max);
	}

	void Canvas::setYRange(Canvas::range_t range)
	{
		UI_LOCK();
		m_yLabels.clear();
		if (m_yLabelPtr != nullptr)
		{
			delete[] m_yLabelPtr;
			m_yLabelPtr = nullptr;
		}
		lv_scale_set_text_src(m_vScale, nullptr);
		lv_scale_set_range(m_vScale, range.min, range.max);
	}

	void Canvas::createLabels(Canvas::range_float_t range,
							  uint32_t ticks,
							  std::vector<std::string>& vec,
							  const char**& labels)
	{
		UI_LOCK();
		vec.clear();
		vec.reserve(ticks);
		float step = (range.max - range.min) / (float)(ticks - 1);
		for (uint32_t i = 0; i < ticks; ++i)
		{
			vec.emplace_back(utils::format("%.2f", range.min + step * (float)i));
		}

		// Delete existing labels if any
		if (labels != nullptr)
		{
			delete[] labels;
			labels = nullptr;
		}

		labels = new const char*[vec.size() + 1];
		for (size_t i = 0; i < vec.size(); ++i)
		{
			labels[i] = vec[i].c_str();
		}
		labels[vec.size()] = nullptr;
	}

	void Canvas::setXRange(Canvas::range_float_t range)
	{
		UI_LOCK();
		uint32_t ticks = 1 + lv_scale_get_total_tick_count(m_hScale) / lv_scale_get_major_tick_every(m_hScale);
		createLabels(range, ticks, m_xLabels, m_xLabelPtr);
		lv_scale_set_range(m_hScale, range.min, range.max);
		lv_scale_set_text_src(m_hScale, m_xLabelPtr);
	}

	void Canvas::setYRange(Canvas::range_float_t range)
	{
		UI_LOCK();
		uint32_t ticks = 1 + lv_scale_get_total_tick_count(m_vScale) / lv_scale_get_major_tick_every(m_vScale);
		createLabels(range, ticks, m_yLabels, m_yLabelPtr);
		lv_scale_set_range(m_vScale, range.min, range.max);
		lv_scale_set_text_src(m_vScale, m_yLabelPtr);
	}

	bool Canvas::pxToPos(size_t px, size_t py, float& x, float& y) const
	{
		UI_LOCK();
		uint32_t res_x, res_y;
		getResolution(res_x, res_y);

		if (px >= res_x || py >= res_y)
		{
			return false;
		}

		range_t xRange = getXRange();
		range_t yRange = getYRange();

		x = xRange.min + (float)px * (xRange.max - xRange.min) / (float)res_x;
		y = yRange.min + (float)py * (yRange.max - yRange.min) / (float)res_y;

		return true;
	}

	bool Canvas::pxToPos(lv_point_t p, float& x, float& y) const
	{
		UI_LOCK();
		return pxToPos(p.x, p.y, x, y);
	}

	bool Canvas::posToPx(float x, float y, size_t& px, size_t& py) const
	{
		UI_LOCK();
		range_t xRange = getXRange();
		range_t yRange = getYRange();

		if (x < xRange.min || x > xRange.max || y < yRange.min || y > yRange.max)
		{
			return false;
		}

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);

		px = (size_t)((x - xRange.min) * (float)res_x / (xRange.max - xRange.min));
		py = (size_t)(res_y - (y - yRange.min) * (float)res_y / (yRange.max - yRange.min));

		return true;
	}

	bool Canvas::posToPx(float x, float y, int32_t& px, int32_t& py) const
	{
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
		UI_LOCK();
		if (m_buf != nullptr)
		{
			lv_draw_buf_destroy(m_buf);
		}
		width = std::max(width, 1u);
		height = std::max(height, 1u);
		m_buf = lv_draw_buf_create(width, height, lv_display_get_color_format(lv_obj_get_display(getRoot())), 0);
		lv_canvas_set_draw_buf(m_canvas, m_buf);
		clear();
	}

	void Canvas::drawGrid()
	{
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
		UI_LOCK();
		// Draw the pixel
		lv_canvas_set_px(getCanvasObj(), px, py, color, LV_OPA_COVER);
	}

	void Canvas::drawRect(lv_area_t area, int32_t radius, lv_color_t color, lv_opa_t opa)
	{
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
		UI_LOCK();
		LOG_DBG("area: ({:d}, {:d}), ({:d}, {:d}), radius: {:d}", area.x1, area.y1, area.x2, area.y2, radius);

		lv_draw_rect_dsc_t dsc;
		lv_draw_rect_dsc_init(&dsc);
		dsc.bg_opa = opa;
		dsc.bg_color = color;
		dsc.radius = radius;

		lv_layer_t layer;
		lv_canvas_init_layer(m_canvas, &layer);

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);
		area.x1 = std::max(area.x1, 0);
		area.y1 = std::max(area.y1, 0);
		area.x2 = std::min(area.x2, (int32_t)res_x - 1);
		area.y2 = std::min(area.y2, (int32_t)res_y - 1);

		lv_draw_rect(&layer, &dsc, &area);

		lv_draw_rect(&layer, &dsc, &area);

		lv_canvas_finish_layer(m_canvas, &layer);
	}

	void Canvas::drawLine(lv_point_t p1, lv_point_t p2, lv_color_t color, lv_opa_t opa)
	{
		UI_LOCK();
		LOG_DBG("p1: ({}, {}), p2: ({}, {})", p1.x, p1.y, p2.x, p2.y);

		if (!posToPx(p1.x, p1.y, p1.x, p1.y))
		{
			LOG_WARN("invalid point ({}, {})", p1.x, p1.y);
			return;
		}
		if (!posToPx(p2.x, p2.y, p2.x, p2.y))
		{
			LOG_WARN("invalid point ({}, {})", p2.x, p2.y);
			return;
		}
		drawLinePx(p1, p2, color, opa);
	}

	void Canvas::drawLinePx(lv_point_t p1, lv_point_t p2, lv_color_t color, lv_opa_t opa)
	{
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
		lv_canvas_init_layer(m_canvas, &layer);

		lv_draw_line(&layer, &line_dsc);

		lv_canvas_finish_layer(m_canvas, &layer);
	}

	void Canvas::drawCircle(lv_point_t center, uint32_t radius, lv_color_t color, lv_opa_t opa)
	{
		UI_LOCK();
		LOG_DBG("center: ({:d}, {:d}), radius: {:d}", center.x, center.y, radius);
		if (!posToPx(center.x, center.y, center.x, center.y))
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
		lv_canvas_init_layer(m_canvas, &layer);

		uint32_t res_x, res_y;
		getResolution(res_x, res_y);

		lv_point_t txt_size;
		lv_text_attributes_t txt_attr;
		lv_text_attributes_init(&txt_attr);
		txt_attr.max_width = std::min((res_x - pos.x) * 2, res_x);
		lv_text_get_size(&txt_size, dsc.text, dsc.font, &txt_attr);

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

		lv_canvas_finish_layer(m_canvas, &layer);
	}

	lv_color_t Canvas::getPx(size_t px, size_t py) const
	{
		lv_color32_t color32 = lv_canvas_get_px(m_canvas, px, py);
		lv_color_t color = {color32.blue, color32.green, color32.red};
		return color;
	}

	void Canvas::clear()
	{
		UI_LOCK();
		lv_style_value_t bg_color;
		lv_style_get_prop(Themes::getLvglStyles().canvas, LV_STYLE_BG_COLOR, &bg_color);
		lv_canvas_fill_bg(m_canvas, bg_color.color, LV_OPA_TRANSP);
	}
} // namespace UI

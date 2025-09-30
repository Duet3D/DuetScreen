/*
 * Graph.cpp
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#include "Graph.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	static constexpr lv_coord_t s_scaleSize = 30;
	static constexpr lv_coord_t s_legendSize = 100;
	static lv_color_t s_hiddenColor = lv_color_darken(lv_color_white(), 50);

	Graph::Graph(const std::string& name, LvObj& parent)
		: LvObj(lv_obj_create, name, parent)
		, m_chart(lv_chart_create(getRoot()))
		, m_vScale(lv_scale_create(getRoot()))
		, m_hScale(lv_scale_create(getRoot()))
		, m_legend("legend", getRoot())
	{
		init();
	}

	Graph::Graph(const std::string& name, LvObj& parent, layout_t layout)
		: LvObj(lv_obj_create, name, parent, layout)
		, m_chart(lv_chart_create(getRoot()))
		, m_vScale(lv_scale_create(getRoot()))
		, m_hScale(lv_scale_create(getRoot()))
		, m_legend("legend", getRoot())
	{
		init();
	}

	void Graph::init()
	{
		UI_LOCK();

		// Layout
		lv_obj_set_layout(getRoot(), LV_LAYOUT_GRID);
		m_columnDsc[0] = s_scaleSize;
		m_columnDsc[1] = LV_GRID_FR(1);
		m_columnDsc[2] = s_legendSize;
		m_columnDsc[3] = LV_GRID_TEMPLATE_LAST;

		m_rowDsc[0] = LV_GRID_FR(1);
		m_rowDsc[1] = s_scaleSize;
		m_rowDsc[2] = LV_GRID_TEMPLATE_LAST;
		lv_obj_set_grid_dsc_array(getRoot(), m_columnDsc, m_rowDsc);
		lv_obj_set_grid_cell(m_vScale, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_hScale, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		lv_obj_set_grid_cell(m_chart, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		lv_obj_set_grid_cell(m_legend, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

		lv_obj_set_style_pad_top(getRoot(), 20, LV_PART_MAIN);
		lv_obj_set_style_pad_right(getRoot(), 20, LV_PART_MAIN);
		lv_obj_set_style_pad_left(getRoot(), 10, LV_PART_MAIN);
		lv_obj_set_style_pad_bottom(getRoot(), 5, LV_PART_MAIN);

		// Chart
		// Horizontal scale
		lv_scale_set_mode(m_hScale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
		lv_scale_set_label_show(m_hScale, true);

		// Vertical scale
		lv_scale_set_mode(m_vScale, LV_SCALE_MODE_VERTICAL_LEFT);
		lv_scale_set_label_show(m_vScale, true);
		lv_scale_set_total_tick_count(m_vScale, 21);
		lv_scale_set_major_tick_every(m_vScale, 4);

		// Chart
		lv_chart_set_type(m_chart, LV_CHART_TYPE_LINE);
		lv_chart_set_update_mode(m_chart, LV_CHART_UPDATE_MODE_SHIFT);

		// Legend
		lv_obj_set_flex_flow(m_legend, LV_FLEX_FLOW_COLUMN);

		showLegend(true);
	}

	void Graph::showLegend(const bool show)
	{
		UI_LOCK();
		m_columnDsc[2] = show ? s_legendSize : 0;
		if (show)
		{
			lv_obj_remove_flag(m_legend, LV_OBJ_FLAG_HIDDEN);
		}
		else
		{
			lv_obj_add_flag(m_legend, LV_OBJ_FLAG_HIDDEN);
		}
		lv_obj_set_grid_dsc_array(getRoot(), m_columnDsc, m_rowDsc);
	}

	Graph::range_t Graph::getXRange() const
	{
		UI_LOCK();
		range_t range;
		range.min = lv_scale_get_range_min_value(m_hScale);
		range.max = lv_scale_get_range_max_value(m_hScale);
		return range;
	}

	Graph::range_t Graph::getYRange() const
	{
		UI_LOCK();
		range_t range;
		range.min = lv_scale_get_range_min_value(m_vScale);
		range.max = lv_scale_get_range_max_value(m_vScale);
		return range;
	}

	void Graph::setXRange(Graph::range_t range)
	{
		UI_LOCK();
		lv_scale_set_range(m_hScale, range.min, range.max);
		lv_chart_set_axis_range(m_chart, LV_CHART_AXIS_PRIMARY_X, range.min, range.max);
	}

	void Graph::setYRange(Graph::range_t range)
	{
		UI_LOCK();
		lv_scale_set_range(m_vScale, range.min, range.max);
		lv_chart_set_axis_range(m_chart, LV_CHART_AXIS_PRIMARY_Y, range.min, range.max);
	}

	void Graph::setXCount(int32_t count)
	{
		UI_LOCK();
		lv_chart_set_point_count(m_chart, count);
	}

	void Graph::setSeriesCount(size_t count)
	{
		UI_LOCK();
		if (count == m_series.size())
		{
			return;
		}
		if (count < m_series.size())
		{
			for (size_t i = count; i < m_series.size(); ++i)
			{
				lv_chart_remove_series(m_chart, m_series[i].series);
			}
			m_series.resize(count);
			return;
		}
		m_series.reserve(count);
	}

	const Graph::series_t* Graph::getSeries(const size_t index)
	{
		if (index >= m_series.size())
		{
			LOG_DBG("Series index out of range");
			return nullptr;
		}
		return &m_series[index];
	}

	bool Graph::createSeries(lv_color_t color, const std::string& displayName)
	{
		UI_LOCK();
		lv_chart_series_t* series = lv_chart_add_series(m_chart, color, LV_CHART_AXIS_PRIMARY_Y);

		if (series == nullptr)
		{
			LOG_ERROR("Failed to create series");
			return false;
		}

		size_t index = getSeriesCount();

		std::shared_ptr<legend_obj_t> legendObj = std::make_shared<legend_obj_t>(
			fmt::format("legend_obj_{}", index), m_legend, displayName.c_str(), layout_t(0, 0, 100, 20));
		legendObj->setStyleBgColor(color, LV_STATE_CHECKED);
		legendObj->addStyle(Themes::getLvglStyles().border);
		legendObj->addStyle(Themes::getLvglStyles().bg_light, LV_STATE_DEFAULT);
		legendObj->setCheckable(true);
		legendObj->setChecked(true);
		legendObj->addClickedCallback(legendEvent, this);
		legendObj->setUserData((void*)(uintptr_t)index);
		m_series.push_back(series_t(series, color, legendObj));
		return true;
	}

	bool Graph::updateSeriesColor(const size_t index, lv_color_t color)
	{
		UI_LOCK();
		series_t* series = (series_t*)getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot update series, series not found");
			return false;
		}
		setSeriesColor(*series, color);
		return true;
	}

	bool Graph::updateSeriesName(const size_t index, const std::string& displayName)
	{
		UI_LOCK();
		series_t* series = (series_t*)getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot update series, series not found");
			return false;
		}
		auto legendObj = series->legendObj;
		legendObj->setText(displayName.c_str());
		return true;
	}

	void Graph::showSeries(const size_t index, const bool show)
	{
		UI_LOCK();
		const series_t* series = getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot show/hide series, series not found");
			return;
		}
		lv_chart_hide_series(m_chart, series->series, !show);
		series->legendObj->setChecked(show);
	}

	void Graph::clear()
	{
		UI_LOCK();
		for (auto& series : m_series)
		{
			lv_chart_remove_series(m_chart, series.series);
		}
		lv_chart_refresh(m_chart);
		m_series.clear();
	}

	void Graph::clear(const size_t index)
	{
		UI_LOCK();
		const series_t* series = getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot clear series, series not found");
			return;
		}
		lv_chart_remove_series(m_chart, series->series);
		lv_chart_refresh(m_chart);
		m_series.erase(m_series.begin() + index);
	}

	void Graph::addData(const size_t index, int32_t value)
	{
		UI_LOCK();
		const series_t* series = getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot add data to series, series not found");
			return;
		}
		lv_chart_set_next_value(m_chart, series->series, value);
	}

	void Graph::legendEvent(lv_event_t* e)
	{
		UI_LOCK();
		Graph* g = (Graph*)lv_event_get_user_data(e);
		lv_obj_t* btn = lv_event_get_target_obj(e);
		size_t index = (uintptr_t)lv_obj_get_user_data(btn);

		// checked is inverted since this callback runs before the state is updated
		g->showSeries(index, lv_obj_has_state(btn, LV_STATE_CHECKED));
	}

	void Graph::setSeriesColor(series_t& series, lv_color_t color)
	{
		UI_LOCK();
		lv_chart_set_series_color(m_chart, series.series, color);
		series.color = color;

		auto legendObj = series.legendObj->getRootPtr();
		lv_obj_set_style_bg_color(legendObj, color, LV_STATE_CHECKED);
		lv_obj_set_style_bg_color(legendObj, s_hiddenColor, LV_STATE_DEFAULT);
	}
} // namespace UI

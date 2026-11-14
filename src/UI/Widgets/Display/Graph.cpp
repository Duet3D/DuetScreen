/*
 * Graph.cpp
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#include "Graph.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include <fmt/format.h>

namespace UI
{
	Graph::LegendItem::LegendItem(const std::string& objName, LvObj& legend, std::string_view name)
		: LvContainer(objName, legend)
	{
		ZoneScoped;
		const auto& lvgl = Themes::getLvglStyles();
		const auto& components = Themes::getComponentStyles();

		// A neutral control: the theme's button, then the legend's own look, which shows whether it is shown
		addStyle(lvgl.btn);
		addStyle(lvgl.pressed, LV_STATE_PRESSED);
		addStyle(lvgl.outline_primary, LV_STATE_FOCUS_KEY);
		addStyle(components.graph_legend_item);
		addStyle(components.graph_legend_item_shown, LV_STATE_CHECKED);
		setFlag(LV_OBJ_FLAG_CLICKABLE, true);
		setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setFlexFlow(LV_FLEX_FLOW_ROW);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_swatch.addStyle(components.graph_legend_swatch);
		m_swatch.addStyle(components.graph_legend_swatch_shown, LV_STATE_CHECKED);
		m_swatch.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		m_swatch.setFlag(LV_OBJ_FLAG_EVENT_BUBBLE, true);

		// The name takes the room left, wrapping if it must; the value keeps its own width
		m_name.setFlexGrow(1);
		m_name.setWidth(1);
		m_name.setLongMode(LV_LABEL_LONG_MODE_WRAP);
		m_name.setText(name);
		m_value.addStyle(components.graph_legend_value);
		m_value.setWidth(LV_SIZE_CONTENT);
		m_value.setVisible(false);

		setShown(true);
	}

	std::string_view Graph::LegendItem::getName() const
	{
		return m_name.getText();
	}

	void Graph::LegendItem::setName(std::string_view name)
	{
		m_name.setText(name);
	}

	void Graph::LegendItem::setValue(std::string_view value)
	{
		m_value.setText(value);
		m_value.setVisible(!value.empty());
	}

	std::string_view Graph::LegendItem::getValue() const
	{
		return m_value.getText();
	}

	void Graph::LegendItem::setColor(lv_color_t color)
	{
		lv_obj_t* swatch = m_swatch.getRootPtr();
		lv_obj_set_style_bg_color(swatch, color, LV_PART_MAIN);
		lv_obj_set_style_border_color(swatch, color, LV_PART_MAIN);
	}

	void Graph::LegendItem::setShown(bool shown)
	{
		setState(LV_STATE_CHECKED, shown);
		m_swatch.setState(LV_STATE_CHECKED, shown);
	}

	Graph::Graph(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		init();
	}

	Graph::Graph(const std::string& name, lv_obj_t* parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		init();
	}

	Graph::~Graph()
	{
		Themes::removeThemeListener(m_themeListener);
	}

	void Graph::init()
	{
		ZoneScoped;
		UI_LOCK();

		// Layout
		setFlexFlow(LV_FLEX_FLOW_ROW);
		m_chartCont.setSize(LV_PCT(100), LV_PCT(100));
		m_chartCont.setFlexGrow(1);

		// The legend's width is the theme's (graph_legend), so it never depends on the names in it
		m_legend.setHeight(LV_PCT(100));
		m_legend.addStyle(Themes::getComponentStyles().graph_legend);
		m_legend.setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_columnDsc = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
		m_rowDsc = {LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

		m_chartCont.setGridDsc(m_columnDsc, m_rowDsc);
		m_chartCont.setGridCell(m_vScale, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		m_chartCont.setGridCell(m_hScale, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 1, 1);
		m_chartCont.setGridCell(m_chart, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

		/* Chart */
		// Horizontal scale
		m_hScale.setHeight(LV_SIZE_CONTENT);
		m_hScale.setMode(LV_SCALE_MODE_HORIZONTAL_BOTTOM);
		m_hScale.setLabelShow(true);

		// Vertical scale
		m_vScale.setWidth(LV_SIZE_CONTENT);
		m_vScale.setMode(LV_SCALE_MODE_VERTICAL_LEFT);
		m_vScale.setLabelShow(true);
		m_vScale.setTotalTickCount(21);
		m_vScale.setMajorTickEvery(4);

		// Chart
		m_chart.setType(LV_CHART_TYPE_LINE);
		m_chart.setUpdateMode(LV_CHART_UPDATE_MODE_SHIFT);

		showLegend(true);

		m_themeListener = Themes::addThemeListener([this] { applyThemeColors(); });
	}

	void Graph::showLegend(const bool show)
	{
		ZoneScoped;
		UI_LOCK();
		m_legend.setVisible(show);
	}

	Graph::range_t Graph::getXRange() const
	{
		ZoneScoped;
		UI_LOCK();
		range_t range;
		range.min = m_hScale.getRangeMinValue();
		range.max = m_hScale.getRangeMaxValue();
		return range;
	}

	Graph::range_t Graph::getYRange() const
	{
		ZoneScoped;
		UI_LOCK();
		range_t range;
		range.min = m_vScale.getRangeMinValue();
		range.max = m_vScale.getRangeMaxValue();
		return range;
	}

	void Graph::setXRange(Graph::range_t range)
	{
		ZoneScoped;
		UI_LOCK();
		m_hScale.setRange(range.min, range.max);
		m_chart.setAxisRange(LV_CHART_AXIS_PRIMARY_X, range.min, range.max);
	}

	void Graph::setYRange(Graph::range_t range)
	{
		ZoneScoped;
		UI_LOCK();
		m_vScale.setRange(range.min, range.max);
		m_chart.setAxisRange(LV_CHART_AXIS_PRIMARY_Y, range.min, range.max);
	}

	void Graph::setXCount(int32_t count)
	{
		ZoneScoped;
		UI_LOCK();
		m_chart.setPointCount(count);
	}

	void Graph::setValueFormat(std::string format)
	{
		m_valueFormat = std::move(format);
	}

	void Graph::setSeriesCount(size_t count)
	{
		ZoneScoped;
		UI_LOCK();
		if (count >= m_series.size())
		{
			return;
		}
		for (size_t i = count; i < m_series.size(); ++i)
		{
			m_chart.removeSeries(m_series[i].series);
		}
		m_series.resize(count);
	}

	const Graph::series_t* Graph::getSeries(const size_t index) const
	{
		if (index >= m_series.size())
		{
			LOG_DBG("Series index out of range");
			return nullptr;
		}
		return &m_series[index];
	}

	bool Graph::createSeries(const std::string& displayName)
	{
		ZoneScoped;
		UI_LOCK();
		const size_t index = getSeriesCount();
		const lv_color_t color = Themes::seriesColor(index);
		lv_chart_series_t* series = m_chart.addSeries(color, LV_CHART_AXIS_PRIMARY_Y);
		if (series == nullptr)
		{
			LOG_ERROR("Failed to create series");
			return false;
		}

		auto item = std::make_unique<LegendItem>(fmt::format("legend_item_{}", index), m_legend, displayName);
		item->setColor(color);
		item->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(index)));
		item->addEventCallback(legendEvent, LV_EVENT_CLICKED, this);
		m_series.push_back({series, std::move(item)});
		return true;
	}

	bool Graph::updateSeriesName(const size_t index, const std::string& displayName)
	{
		ZoneScoped;
		UI_LOCK();
		const series_t* series = getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot update series, series not found");
			return false;
		}
		series->legendItem->setName(displayName);
		return true;
	}

	void Graph::showSeries(const size_t index, const bool show)
	{
		ZoneScoped;
		UI_LOCK();
		const series_t* series = getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot show/hide series, series not found");
			return;
		}
		m_chart.hideSeries(series->series, !show);
		series->legendItem->setShown(show);
	}

	bool Graph::isSeriesShown(const size_t index) const
	{
		const series_t* series = getSeries(index);
		return series != nullptr && series->legendItem->hasState(LV_STATE_CHECKED);
	}

	void Graph::clear()
	{
		ZoneScoped;
		UI_LOCK();
		for (auto& series : m_series)
		{
			m_chart.removeSeries(series.series);
		}
		m_chart.refresh();
		m_series.clear();
	}

	void Graph::addData(const size_t index, int32_t value)
	{
		ZoneScoped;
		UI_LOCK();
		const series_t* series = getSeries(index);
		if (series == nullptr)
		{
			LOG_WARN("Cannot add data to series, series not found");
			return;
		}
		m_chart.setNextValue(series->series, value);
		series->legendItem->setValue(formatValue(value));
	}

	void Graph::legendEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		auto* graph = static_cast<Graph*>(lv_event_get_user_data(e));
		const LvObj* item = LvObj::fromPtr(lv_event_get_current_target_obj(e));
		graph->onLegendClicked(static_cast<size_t>(reinterpret_cast<uintptr_t>(item->getUserData())));
	}

	void Graph::onLegendClicked(size_t index)
	{
		showSeries(index, !isSeriesShown(index));
	}

	void Graph::applyThemeColors()
	{
		ZoneScoped;
		UI_LOCK();
		for (size_t i = 0; i < m_series.size(); i++)
		{
			const lv_color_t color = Themes::seriesColor(i);
			m_chart.setSeriesColor(m_series[i].series, color);
			m_series[i].legendItem->setColor(color);
		}
		m_chart.refresh();
	}

	std::string Graph::formatValue(int32_t value) const
	{
		if (m_valueFormat.empty())
		{
			return "";
		}
		try
		{
			return fmt::format(fmt::runtime(m_valueFormat), value);
		}
		catch (const fmt::format_error& e)
		{
			LOG_WARN("Graph value format \"{:s}\": {:s}", m_valueFormat, e.what());
			return "";
		}
	}
} // namespace UI

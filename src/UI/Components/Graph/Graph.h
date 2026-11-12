/*
 * Graph.h
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvChart.h"
#include "UI/Components/LVGL/LvScale.h"
#include "UI/Core/View.h"
#include <map>
#include <memory>

namespace UI
{
	/**
	 * A line chart with axes and a legend. Each series has a legend button that shows or hides it. Values are added
	 * at the right and scroll left.
	 */
	class Graph : public LvContainer
	{
	  public:
		typedef Button legend_obj_t;

		/** One line of the chart */
		struct series_t
		{
			lv_chart_series_t* series;				 // The chart's series
			lv_color_t color;						 // Line colour
			std::unique_ptr<legend_obj_t> legendObj; // Its legend button
		};
		typedef std::vector<series_t> series_list_t;

		/** An axis range */
		struct range_t
		{
			int32_t min; // Lowest value
			int32_t max; // Highest value
		};

		/** Create an empty graph inside `parent` */
		Graph(const std::string& name, LvObj& parent);

		/** Create an empty graph inside a widget that has no C++ view, e.g. one built by the XML engine */
		Graph(const std::string& name, lv_obj_t* parent);

		/** Show or hide the legend */
		void showLegend(const bool show);

		/** The X axis range */
		range_t getXRange() const;

		/** The Y axis range */
		range_t getYRange() const;

		/** Set the X axis range (labels only: the chart shows getXCount() values across it) */
		void setXRange(range_t range);

		/** Set the Y axis range */
		void setYRange(range_t range);

		/** Set how many values each series shows across the X axis */
		void setXCount(int32_t count);

		/** Remove series from the end until there are at most `count` */
		void setSeriesCount(size_t count);

		/** Number of series */
		size_t getSeriesCount() const { return m_series.size(); }

		/** The series at `index`, or nullptr */
		const series_t* getSeries(const size_t index);

		/**
		 * Add a series, shown, at the end
		 * @param color Line and legend colour
		 * @param displayName Legend text
		 * @return false if the chart couldn't create it
		 */
		bool createSeries(lv_color_t color, const std::string& displayName);

		/** Change a series' colour. @return false if there is no such series */
		bool updateSeriesColor(const size_t index, lv_color_t color);

		/** Change a series' legend text. @return false if there is no such series */
		bool updateSeriesName(const size_t index, const std::string& displayName);

		/** Show or hide a series, as its legend button does */
		void showSeries(const size_t index, const bool show);

		/** Remove every series */
		void clear();

		/** Remove the series at `index`; later series move down one */
		void clear(const size_t index);

		/** Add `value` at the right of a series, scrolling it left */
		void addData(const size_t index, int32_t value);

	  private:
		/** A legend button was clicked: show or hide its series */
		static void legendEvent(lv_event_t* e);

		/** Lay out and style the parts */
		void init();

		/** Set a series' line and legend colour */
		void setSeriesColor(series_t& series, lv_color_t color);

		LvContainer m_chartCont{"chart_cont", getRoot()}; // The chart and its scales
		LvChart m_chart{"chart", m_chartCont};			  // The lines
		LvScale m_vScale{"vscale", m_chartCont};			  // Y axis
		LvScale m_hScale{"hscale", m_chartCont};			  // X axis
		LvContainer m_legend{"legend", getRoot()};		  // One button per series

		std::array<int32_t, 3> m_columnDsc; // m_chartCont grid columns
		std::array<int32_t, 3> m_rowDsc;	// m_chartCont grid rows

		series_list_t m_series; // The series, in legend order
	};
} // namespace UI

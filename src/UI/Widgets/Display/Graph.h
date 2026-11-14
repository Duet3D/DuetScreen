/*
 * Graph.h
 *
 *  Created on: 2025-01-20
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvChart.h"
#include "UI/Lvgl/LvContainer.h"
#include "UI/Lvgl/LvLabel.h"
#include "UI/Lvgl/LvScale.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace UI
{
	/**
	 * A line chart with axes and a legend. Values are added at the right and scroll left.
	 *
	 * Each series is drawn in the theme's series colours in turn (#series_1, #series_2, …; see Palette.h), and
	 * recoloured when the theme changes. Its legend entry shows its colour, its name and its latest value, and shows
	 * or hides it when tapped. The legend is styled by the theme: graph_legend, graph_legend_item (an entry, as it looks
	 * while its series is hidden), graph_legend_item_shown, graph_legend_swatch, graph_legend_swatch_shown and
	 * graph_legend_value.
	 */
	class Graph : public LvContainer
	{
	  public:
		/** One series' entry in the legend: its colour, its name and its latest value */
		class LegendItem : public LvContainer
		{
		  public:
			/** An entry in `legend` called `name`, shown */
			LegendItem(const std::string& objName, LvObj& legend, std::string_view name);

			/** The series' name */
			std::string_view getName() const;

			/** Show the series' name */
			void setName(std::string_view name);

			/** Show the series' latest value, already formatted */
			void setValue(std::string_view value);

			/** The latest value, as shown */
			std::string_view getValue() const;

			/** Draw the colour key in `color` */
			void setColor(lv_color_t color);

			/** Look shown (true) or hidden */
			void setShown(bool shown);

		  private:
			LvContainer m_swatch{"swatch", *this}; // The series' colour
			LvLabel m_name{"name", *this};		   // The series' name
			LvLabel m_value{"value", *this};	   // Its latest value
		};

		/** One line of the chart */
		struct series_t
		{
			lv_chart_series_t* series;				 // The chart's series
			std::unique_ptr<LegendItem> legendItem; // Its entry in the legend
		};

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

		/** Stop following theme changes */
		~Graph() override;

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

		/**
		 * How the legend shows a series' latest value: a fmt format with one argument, the value, e.g. "{}°C". Empty
		 * shows no value.
		 */
		void setValueFormat(std::string format);

		/** Remove series from the end until there are at most `count` */
		void setSeriesCount(size_t count);

		/** Number of series */
		size_t getSeriesCount() const { return m_series.size(); }

		/** The series at `index`, or nullptr */
		const series_t* getSeries(const size_t index) const;

		/** The colour series `index` is drawn in: the theme's series colours in turn */
		static lv_color_t seriesColor(size_t index);

		/**
		 * Add a series, shown, at the end, in the next of the theme's series colours
		 * @param displayName Legend text
		 * @return false if the chart couldn't create it
		 */
		bool createSeries(const std::string& displayName);

		/** Change a series' legend text. @return false if there is no such series */
		bool updateSeriesName(const size_t index, const std::string& displayName);

		/** Show or hide a series, as its legend entry does */
		void showSeries(const size_t index, const bool show);

		/** Whether a series is shown */
		bool isSeriesShown(const size_t index) const;

		/** Remove every series */
		void clear();

		/** Add `value` at the right of a series, scrolling it left, and show it in the legend */
		void addData(const size_t index, int32_t value);

	  private:
		/** A legend entry was clicked: show or hide its series */
		static void legendEvent(lv_event_t* e);

		/** Lay out and style the parts */
		void init();

		/** Colour every series from the theme in use */
		void applyThemeColors();

		/** `value` in the value format, or empty if there is none or it doesn't fit */
		std::string formatValue(int32_t value) const;

		LvContainer m_chartCont{"chart_cont", getRoot()}; // The chart and its scales
		LvChart m_chart{"chart", m_chartCont};			  // The lines
		LvScale m_vScale{"vscale", m_chartCont};		  // Y axis
		LvScale m_hScale{"hscale", m_chartCont};		  // X axis
		LvContainer m_legend{"legend", getRoot()};		  // One entry per series

		std::array<int32_t, 3> m_columnDsc; // m_chartCont grid columns
		std::array<int32_t, 3> m_rowDsc;	// m_chartCont grid rows

		std::vector<series_t> m_series; // The series, in legend order
		std::string m_valueFormat;		// How the legend shows a value; empty for none
		size_t m_themeListener;			// Recolours the series when the theme changes
	};
} // namespace UI

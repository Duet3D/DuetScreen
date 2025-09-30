/*
 * Graph.h
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

	class Graph : public LvObj
	{
	  public:
		typedef Button legend_obj_t;
		struct series_t
		{
			lv_chart_series_t* series;
			lv_color_t color;
			std::shared_ptr<legend_obj_t> legendObj;
		};
		typedef std::vector<series_t> series_list_t;
		struct range_t
		{
			int32_t min;
			int32_t max;
		};

		Graph(const std::string& name, LvObj& parent);
		Graph(const std::string& name, LvObj& parent, layout_t layout);

		void showLegend(const bool show);

		range_t getXRange() const;
		range_t getYRange() const;
		void setXRange(range_t range);
		void setYRange(range_t range);
		void setXCount(int32_t count);

		void setSeriesCount(size_t count);
		size_t getSeriesCount() const { return m_series.size(); }
		const series_t* getSeries(const size_t index);
		bool createSeries(lv_color_t color, const std::string& displayName);
		bool updateSeriesColor(const size_t index, lv_color_t color);
		bool updateSeriesName(const size_t index, const std::string& displayName);
		void showSeries(const size_t index, const bool show);

		void clear();
		void clear(const size_t index);
		void addData(const size_t index, int32_t value);

	  private:
		static void legendEvent(lv_event_t* e);

		void init();
		void setSeriesColor(series_t& series, lv_color_t color);

		lv_obj_t* m_chart;
		lv_obj_t* m_vScale;
		lv_obj_t* m_hScale;
		LvContainer m_legend;

		int32_t m_columnDsc[4];
		int32_t m_rowDsc[3];

		series_list_t m_series;
	};
} // namespace UI

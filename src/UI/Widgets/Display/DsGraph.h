/*
 * DsGraph.h
 *
 * <ds_graph>: a line graph of a History (see Binding/History.h), with a legend whose entries show and hide series. Its
 * attributes are in registerXml().
 *
 * `legend` shows the legend always, never, or (`auto`) only while the graph is at least `legend_min_width` wide, so a
 * narrow graph keeps its room for the plot. The graph's width is set by its parent, never by the legend, so this can't
 * make it resize.
 */

#pragma once

#include "UI/Widgets/Display/Graph.h"
#include "UI/Xml/Binding/History.h"
#include "helix_xml.h"
#include <string>
#include <vector>

namespace UI::Xml
{
	class DsGraph : public Graph, private ListBase::Listener, private History::SampleListener
	{
	  public:
		/** Create an empty graph inside `parent` */
		DsGraph(const std::string& name, lv_obj_t* parent);

		/** Stop following the history */
		~DsGraph() override;

		/** Register `<ds_graph>` with the XML engine */
		static void registerXml();

		/**
		 * Show the last `seconds` of `history`, which must outlive the graph or be destroyed first (the graph lets go
		 * when it is). Replaces anything shown.
		 */
		void showHistory(History& history, uint32_t seconds);

		/** Set the Y axis range. The top is raised to the bind_y_max subject's value if that is higher. */
		void setYAxis(range_t range);

	  private:
		/** `<ds_graph>` create callback: create the C++ graph, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_graph>` apply callback: lv_obj attributes, then the graph's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A series' name changed: update its legend */
		static void nameObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_y_max` observer */
		static void yMaxObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** The graph changed size: show the legend if it is wide enough for it */
		static void sizeChangedCb(lv_event_t* e);

		/** When the legend shows */
		enum class LegendMode
		{
			Auto,	// While the graph is at least `legend_min_width` wide
			Shown,	// Always
			Hidden, // Never
		};

		/** Show the legend as its mode says */
		void fitLegend();

		/** Series were added to or removed from the history */
		void onListResized(size_t oldSize, size_t newSize) override;

		/** The history is being destroyed: remove every series and let go of it */
		void onListDestroyed() override;

		/** Add the newest value of every series */
		void onSampled() override;

		/** Add series for history rows [from, to), with the values recorded so far */
		void addSeries(size_t from, size_t to);

		/** Remove series until `count` remain */
		void removeSeries(size_t count);

		/** Stop following the history and remove every series */
		void forgetHistory();

		/** Set the Y axis from the configured range and the bound limit */
		void applyYAxis();

		History* m_history = nullptr;				 // The history shown, or nullptr
		size_t m_points = 0;						 // Values shown across the X axis
		std::vector<lv_observer_t*> m_nameObservers; // One per series, on its row's name
		range_t m_yAxis{0, 100};					 // Configured Y range
		int32_t m_yLimit = 0;						 // Value of the bind_y_max subject, or 0
		int32_t m_legendMinWidth = 0;				 // Narrowest width the legend shows at, pixels; 0: always
		LegendMode m_legendMode = LegendMode::Auto;	 // When the legend shows
	};
} // namespace UI::Xml

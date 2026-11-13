/*
 * DsGraph.h
 *
 * <ds_graph>: a line graph of a History (see Binding/History.h), with a legend whose buttons show and hide series. Its
 * attributes are in registerXml().
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
	};
} // namespace UI::Xml

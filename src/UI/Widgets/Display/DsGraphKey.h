/*
 * DsGraphKey.h
 *
 * <ds_graph_key>: one series' colour key, as a graph's legend draws it, shown beside what the series measures, e.g. a
 * heater's row in a tool list, linking it to its line in a `<ds_graph>` of the same history. Its attributes are in
 * registerXml().
 *
 * The series is the history row whose `row.key` is the key given, so it is found however the series are ordered; the
 * key's colour is that row's position in the theme's series colours (Themes::seriesColor()), as the graph colours it.
 * Tapping it shows or hides the series in every graph of the history (the row's `shown`); it is hollow while the
 * series is hidden. It hides itself while no series has its key, and always while it isn't enabled (e.g. turned off
 * by a widget setting): it owns its `hidden` flag, so that is how XML turns it off.
 */

#pragma once

#include "UI/Lvgl/LvContainer.h"
#include "UI/Xml/Binding/History.h"
#include "helix_xml.h"
#include <optional>
#include <string>
#include <vector>

namespace UI::Xml
{
	class DsGraphKey : public LvContainer, private ListBase::Listener
	{
	  public:
		/** Create a key inside `parent`, hidden until it finds its series */
		DsGraphKey(const std::string& name, lv_obj_t* parent);

		/** Stop following the history and the theme */
		~DsGraphKey() override;

		/** Register `<ds_graph_key>` with the XML engine */
		static void registerXml();

		/**
		 * Look for the series in `history`, which must outlive the key or be destroyed first (the key lets go when it
		 * is). Replaces any history followed before.
		 */
		void showHistory(History& history);

		/** Show the series whose row.key is `key` */
		void setKey(int32_t key);

		/** Show the key (true, the default) while its series is there, or never (false) */
		void setEnabled(bool enabled);

	  private:
		/** `<ds_graph_key>` create callback: create the C++ key, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_graph_key>` apply callback: lv_obj attributes, then the key's own */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_key` observer */
		static void keyObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** A row's key or shown changed: find the series again */
		static void rowObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Tapped: show or hide the series */
		static void clickedCb(lv_event_t* e);

		/** Series were added to the history, or are about to be removed: observe the rows there will be */
		void onListResized(size_t oldSize, size_t newSize) override;

		/** The history is being destroyed: let go of its rows and of it */
		void onListDestroyed() override;

		/** The position of the row observed whose key is m_key, or nullopt */
		std::optional<size_t> findSeries() const;

		/** Show the series' colour and whether it is shown, or hide while there is none */
		void refresh();

		/** Observe the key and shown of the history's first `count` rows, and no others */
		void observeRows(size_t count);

		/** Stop following the history */
		void forgetHistory();

		/** What the key follows in one history row */
		struct RowObservers
		{
			lv_observer_t* key;	  // On row.key
			lv_observer_t* shown; // On row.shown
		};

		History* m_history = nullptr;	  // The history the series is in, or nullptr
		int32_t m_key = -1;				  // The row.key of the series shown
		bool m_enabled = true;			  // Shown at all: false hides it whatever the series
		std::vector<RowObservers> m_rows; // One per history row observed, in order
		size_t m_themeListener;			  // Recolours the key when the theme changes
	};
} // namespace UI::Xml

/*
 * DsList.h
 *
 * <ds_list>: one component per row of a List (see Binding/List.h). Its attributes are in registerXml().
 *
 * A keyed list (`key="number" value="2"`) shows only the rows whose `key` field equals `value`: one fan, a tool's
 * card, every active heater. Each item is given the list's `item.*` attributes (see ItemAttributes.h). With
 * `flow="pages"` it shows one item at a time (see ListPages.h). It watches each row's key, so the rows shown follow the data even when a row is
 * refilled with another item (List::sync() fills rows by position, so removing fan 1 moves fan 2 into its row).
 */

#pragma once

#include "ItemAttributes.h"
#include "ListPages.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Scope.h"
#include "UI/Xml/Binding/Subjects.h"
#include "helix_xml.h"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Xml
{
	class DsList : public ListBase::Listener
	{
	  public:
		/** How a windowed list sizes and places its items */
		struct Window
		{
			std::optional<int32_t> itemWidth; // Item width in pixels; the list's width if not set
			bool contentWidth = false;		  // Items keep their own width (column flow)
			int32_t itemHeight = 0;			  // Item height in pixels
			bool wrap = false;				  // Several items per line, as many as fit
		};

		/** Which rows a keyed list shows */
		struct Key
		{
			std::string field; // Name of the row field compared, e.g. "number"
			std::string value; // The value it must have, as XML writes it, e.g. "2"
		};

		/**
		 * Show `list` in `container`, one `item` component per row
		 * @param parentContext Context the list is built in; each row's scope is its child
		 * @param itemAttributes Given to every item, as if written where it is used
		 * @param window Build only the items on screen, placed like this; build every item if not given
		 * @param key Show only the rows whose field `key.field` is `key.value`; every row if not given. Not with
		 *        `window`. The rows must have the field (see keyOf()).
		 * @param pages Show one item at a time (see ListPages.h). Not with `window`.
		 * @param limit Show only the first `limit` rows; every row if not given
		 */
		DsList(lv_obj_t* container,
			   ListBase& list,
			   std::string item,
			   lv_xml_subject_ctx_t* parentContext,
			   ItemAttributes itemAttributes = {},
			   std::optional<Window> window = std::nullopt,
			   bool followEnd = false,
			   std::optional<Key> key = std::nullopt,
			   bool pages = false,
			   std::optional<size_t> limit = std::nullopt);

		/** Delete the rows' widgets, then their scopes, and stop listening; runs while LVGL deletes the container */
		~DsList() override;

		/** Register `<ds_list>` with the XML engine */
		static void registerXml();

		/** Build widgets for added rows, or delete those of removed rows */
		void onListResized(size_t oldSize, size_t newSize) override;

		/** Delete every row widget and forget the list */
		void onListDestroyed() override;

	  private:
		/** A row's widget and the scope it was built in */
		struct Entry
		{
			lv_obj_t* obj = nullptr;			  // Root widget of the item component, or nullptr if not built
			std::unique_ptr<Scope> scope;		  // The row's fields, while the item is built
			Subject* key = nullptr;				  // Keyed: the row's key field
			lv_observer_t* keyObserver = nullptr; // Keyed: watches `key`
		};

		/** `<ds_list>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/**
		 * Lay `container` out for `flow` (see registerXml()): a flex flow, or none for "" or a windowed list
		 * @return whether it shows a page at a time, or nothing if `flow` isn't one of the flows
		 */
		static std::optional<bool> applyFlow(lv_obj_t* container, std::string_view flow, bool windowed);

		/**
		 * `<ds_list>` apply callback: lv_obj attributes, then start showing the list. Applied again to a ds_list
		 * already showing one (a component's `<view extends="ds_list">` gets its instance's attributes too), it only
		 * applies the lv_obj attributes: the first apply's list, item and the rest stay.
		 */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** The field of `row` called `name`, or nullptr if it has none or it is a list */
		static Subject* keyOf(Row& row, std::string_view name);

		/** Keyed: a row's key changed; show or remove its item */
		static void keyObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Keyed with `bind_value`: the value the rows' keys must have changed; show the rows that have it now */
		static void valueObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** A subject's value as XML writes one: an int's digits, a text's text */
		static std::string valueText(lv_subject_t* subject);

		/** A windowed list scrolled or changed size: show the items now on screen */
		static void windowEventCb(lv_event_t* e);

		/**
		 * follow_end: scrolled (note whether the user left the end) or laid out (go back to the end once every size
		 * is final)
		 */
		static void followEndEventCb(lv_event_t* e);

		/**
		 * follow_end: the display finished laying out, so the content's size is final: go back to the end if the
		 * list's layout changed. Scrolling while the layout is still running would use sizes not yet worked out,
		 * e.g. a new wrapped line as tall as it would be one character wide, and leave the list past its end.
		 */
		static void layoutCompletedEventCb(lv_event_t* e);

		/** Position and size row `index`'s item for the window */
		void placeItem(lv_obj_t* obj, size_t index, int32_t perLine) const;

		/** Build the widget for row `index` */
		Entry buildRow(size_t index);

		/** Delete a row's widget */
		static void deleteRow(Entry& entry);

		/** Eager: build widgets for rows [from, to), or keyed, watch their keys and build those that match */
		void build(size_t from, size_t to);

		/** Eager: forget rows, deleting their widgets, until `count` remain */
		void truncate(size_t count);

		/** The items changed: tell the pages, if shown a page at a time */
		void itemsChanged();

		/** Keyed: build row `index`'s item if its key matches, else delete it */
		void showIfMatches(size_t index);

		/** Keyed: the number of items built for rows before `index`, which is where row `index`'s item goes */
		uint32_t itemsBefore(size_t index) const;

		/** Windowed: size the scrollable area for every row, then build the items on screen and delete the rest */
		void updateWindow();

		/** Windowed: the rows [first, last) whose line is at least partly on screen */
		std::pair<size_t, size_t> visibleRows(int32_t perLine) const;

		/** Windowed: build the items on screen that aren't built yet */
		void addVisibleRows(int32_t perLine);

		/** Windowed: the position and size of row `index`'s item */
		lv_area_t itemArea(size_t index, int32_t perLine) const;

		/** Windowed: items per line */
		int32_t itemsPerLine() const;

		/** How many of `rows` rows are shown: all of them, or the first `limit` */
		size_t shownOf(size_t rows) const;

		lv_obj_t* m_container;				   // The ds_list widget
		ListBase* m_list;					   // The list shown, or nullptr once it is destroyed
		std::string m_item;					   // Component built for each row
		ItemAttributes m_itemAttributes;	   // Given to every item
		lv_xml_subject_ctx_t* m_parentContext; // Context the list was built in; holds a reference
		std::vector<Entry> m_entries;		   // Eager: one per row
		std::optional<Key> m_key;			   // Keyed: which rows are shown; nullopt shows every row
		std::optional<size_t> m_limit;		   // Only the first rows are shown, this many; nullopt shows every row

		std::optional<Window> m_window;	 // Windowed: how items are placed; nullopt if eager
		std::map<size_t, Entry> m_shown; // Windowed: the items built, by row
		lv_obj_t* m_sizer = nullptr;	 // Windowed: invisible widget giving the scrollable area its size
		size_t m_count = 0;				 // Windowed: rows to show (the list's length as last told)
		bool m_followEnd = false;		 // Keep showing the last rows
		bool m_atEnd = true;			 // follow_end: the user hasn't scrolled away from the end
		bool m_laidOut = false;			 // follow_end: laid out since the display last finished laying out
		lv_display_t* m_display = nullptr; // follow_end: the display telling us when layout is done
		std::unique_ptr<ListPages> m_pages; // flow="pages": shows the items one at a time; nullptr otherwise
	};
} // namespace UI::Xml

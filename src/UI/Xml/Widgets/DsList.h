/*
 * DsList.h
 *
 * <ds_list>: one component per row of a List (see Binding/List.h). Its attributes are in registerXml().
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Scope.h"
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

		/**
		 * Show `list` in `container`, one `item` component per row
		 * @param parentContext Context the list is built in; each row's scope is its child
		 * @param window Build only the items on screen, placed like this; build every item if not given
		 */
		DsList(lv_obj_t* container,
			   ListBase& list,
			   std::string item,
			   lv_xml_subject_ctx_t* parentContext,
			   std::optional<Window> window = std::nullopt,
			   bool followEnd = false);

		/** Stop listening; runs while LVGL deletes the container, which deletes the rows' widgets next */
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
			lv_obj_t* obj;				  // Root widget of the item component, or nullptr if it failed to build
			std::unique_ptr<Scope> scope; // The row's fields
		};

		/** `<ds_list>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_list>` apply callback: lv_obj attributes, then start showing the list */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A windowed list scrolled or changed size: show the items now on screen */
		static void windowEventCb(lv_event_t* e);

		/** follow_end: scrolled (note whether the user left the end) or laid out (go back to the end) */
		static void followEndEventCb(lv_event_t* e);

		/** Position and size row `index`'s item for the window */
		void placeItem(lv_obj_t* obj, size_t index, int32_t perLine) const;

		/** Build the widget for row `index` */
		Entry buildRow(size_t index);

		/** Delete a row's widget */
		static void deleteRow(Entry& entry);

		/** Eager: build widgets for rows [from, to) */
		void build(size_t from, size_t to);

		/** Eager: delete widgets until `count` remain */
		void truncate(size_t count);

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

		lv_obj_t* m_container;				   // The ds_list widget
		ListBase* m_list;					   // The list shown, or nullptr once it is destroyed
		std::string m_item;					   // Component built for each row
		lv_xml_subject_ctx_t* m_parentContext; // Context the list was built in; holds a reference
		std::vector<Entry> m_entries;		   // Eager: one per row shown

		std::optional<Window> m_window;	 // Windowed: how items are placed; nullopt if eager
		std::map<size_t, Entry> m_shown; // Windowed: the items built, by row
		lv_obj_t* m_sizer = nullptr;	 // Windowed: invisible widget giving the scrollable area its size
		size_t m_count = 0;				 // Windowed: rows to show (the list's length as last told)
		bool m_followEnd = false;		 // Keep showing the last rows
		bool m_atEnd = true;			 // follow_end: the user hasn't scrolled away from the end
	};
} // namespace UI::Xml

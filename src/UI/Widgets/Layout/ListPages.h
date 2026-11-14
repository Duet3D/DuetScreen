/*
 * ListPages.h
 *
 * A `<ds_list flow="pages">` shows one item at a time: each item fills the list, the user swipes sideways between
 * them, and a dot per item below them says which is shown. A dot can be tapped to go to its item. Hidden items (a
 * list's gaps) are no pages. With one page or none, there are no dots and the item has the whole list.
 *
 * The dots are a floating child of the list, kept after its items so they never change the items' positions; the
 * list tells refresh() whenever it adds or removes items, and items shown or hidden are noticed here. The theme
 * styles the dots: list_pages (the row of dots), list_page_dot (a dot) and list_page_dot_current (the shown item's).
 */

#pragma once

#include "helix_xml.h"
#include <cstdint>
#include <vector>

namespace UI::Xml
{
	class ListPages
	{
	  public:
		/** Show the items of `container`, a ds_list's, one at a time; runs until the container is deleted */
		explicit ListPages(lv_obj_t* container);

		ListPages(const ListPages&) = delete;
		ListPages& operator=(const ListPages&) = delete;

		/** Stop listening to the container */
		~ListPages();

		/** The items were added, removed or reordered: size them, and show a dot for each */
		void refresh();

	  private:
		/** The container's items: its children but the dots; only those not hidden if `shownOnly` */
		std::vector<lv_obj_t*> items(bool shownOnly) const;

		/** The pages: the items not hidden, in order */
		std::vector<lv_obj_t*> pages() const;

		/** Size every item to the list, less the room the dots take */
		void sizeItems(const std::vector<lv_obj_t*>& items);

		/** Make the number of dots `count`, and mark the shown item's */
		void updateDots(size_t count);

		/** The index of the page shown, from how far the list is scrolled */
		size_t shownIndex(const std::vector<lv_obj_t*>& items) const;

		/** The height the dots take, with their padding; 0 if they aren't shown */
		int32_t dotsHeight() const;

		/** The list changed size: size the items again */
		static void sizeChangedCb(lv_event_t* e);

		/** Scrolling ended: mark the shown item's dot */
		static void scrollEndCb(lv_event_t* e);

		/** An item was shown or hidden, or the children reordered: count the pages again */
		static void childChangedCb(lv_event_t* e);

		/** A dot was tapped: show its item */
		static void dotClickedCb(lv_event_t* e);

		lv_obj_t* m_container; // The ds_list
		lv_obj_t* m_dots;	   // The row of dots, a floating child of the container
		size_t m_shown = 0;	   // Index of the page shown
		bool m_refreshing = false; // In refresh(), which moving the dots calls again
	};
} // namespace UI::Xml

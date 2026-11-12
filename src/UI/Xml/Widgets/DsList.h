/*
 * DsList.h
 *
 * <ds_list>: one component per row of a List (see Binding/List.h).
 *
 *     <ds_list list="tools" item="tool_card" flex_flow="column"/>
 *
 * `list` is looked up like a subject: the enclosing rows first (so a tool card can show `list="heaters"`, its own
 * tool's heaters), then the view's presenter, then the globals. Each `item` is built in a scope of its own holding
 * its row's fields, whose parent is the scope the list was built in. The ds_list itself is a plain lv_obj container
 * and takes every lv_obj attribute.
 */

#pragma once

#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Scope.h"
#include "helix_xml.h"
#include <memory>
#include <string>
#include <vector>

namespace UI::Xml
{
	class DsList : public ListBase::Listener
	{
	  public:
		/**
		 * Show `list` in `container`, one `item` component per row
		 * @param parentContext Context the list is built in; each row's scope is its child
		 */
		DsList(lv_obj_t* container, ListBase& list, std::string item, lv_xml_subject_ctx_t* parentContext);

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
			lv_obj_t* obj;				 // Root widget of the item component, or nullptr if it failed to build
			std::unique_ptr<Scope> scope; // The row's fields
		};

		/** `<ds_list>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_list>` apply callback: lv_obj attributes, then start showing the list */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** Build widgets for rows [from, to) */
		void build(size_t from, size_t to);

		/** Delete widgets until `count` remain */
		void truncate(size_t count);

		lv_obj_t* m_container;				   // The ds_list widget
		ListBase* m_list;					   // The list shown, or nullptr once it is destroyed
		std::string m_item;					   // Component built for each row
		lv_xml_subject_ctx_t* m_parentContext; // Context the list was built in; holds a reference
		std::vector<Entry> m_entries;		   // One per row shown
	};
} // namespace UI::Xml

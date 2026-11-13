/*
 * DsLayout.h
 *
 * <ds_layout slot="dashboard">: shows the widgets the layout of a slot places (see UI/Xml/Layout/Layout.h). Its
 * attributes are in registerXml().
 *
 * Each placed widget is an XmlView of the widget's component, with the widget's props and settings as the component's
 * props, so a widget with a presenter has one of its own. A grid layout makes the ds_layout a grid of equal cells and
 * puts each widget in its rectangle; a strip leaves the ds_layout's own layout (e.g. `flex_flow="column"`) to place
 * them in order.
 *
 * The ds_layout follows its slot's layout: when it changes (see UI/Xml/Layout/LayoutStore.h) only the widgets that
 * changed are rebuilt. A widget that moved keeps its state; one whose settings changed is built again.
 */

#pragma once

#include "UI/Xml/Layout/Layout.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "helix_xml.h"
#include <map>
#include <string>
#include <vector>

namespace UI::Xml
{
	class XmlView;

	class DsLayout : public LayoutListener
	{
	  public:
		/** Show the layout of `slot` in `container` */
		DsLayout(lv_obj_t* container, std::string slot);

		/** Stop following the layout; runs while LVGL deletes the container, which deletes the widgets next */
		~DsLayout() override;

		DsLayout(const DsLayout&) = delete;
		DsLayout& operator=(const DsLayout&) = delete;

		/** Register `<ds_layout>` with the XML engine */
		static void registerXml();

		/** Show the slot's new layout, if it is this one's */
		void onLayoutChanged(std::string_view slot) override;

		/** The slot shown */
		const std::string& getSlot() const { return m_slot; }

		/** The ds_layout widget */
		lv_obj_t* getContainer() const { return m_container; }

		/** The root widget of the placed item `id`, or nullptr */
		lv_obj_t* findItem(std::string_view id) const;

		/** A live ds_layout showing `slot`, or nullptr if none is built */
		static DsLayout* find(std::string_view slot);

	  private:
		/** One placed widget */
		struct Placement
		{
			LayoutItem item;							// What was placed, as last shown
			std::string component;						// The component built
			std::map<std::string, std::string> props;	// The props it was built with
			XmlView* view = nullptr;					// The view showing it; deleted with its widgets
			lv_obj_t* root = nullptr;					// The view's root widget; nullptr once deleted
		};

		/** `<ds_layout>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_layout>` apply callback: lv_obj attributes, then start showing the slot's layout */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A placed widget's root widget is being deleted: forget it */
		static void viewDeletedCb(lv_event_t* e);

		/** Show the slot's current layout, rebuilding only what changed */
		void refresh();

		/** Make the container a grid of the layout's cells */
		void applyGrid(const Layout& layout);

		/** Build the widget for `item` at position `index` among the placed widgets */
		Placement build(const LayoutItem& item, uint32_t index);

		/** Put a grid item's widget in its cells */
		static void placeInGrid(lv_obj_t* obj, const LayoutItem& item);

		/** Delete a placed widget */
		static void remove(Placement& placement);

		lv_obj_t* m_container;				  // The ds_layout widget
		std::string m_slot;					  // The slot shown
		std::vector<Placement> m_placements;  // The placed widgets, in order
		std::vector<int32_t> m_columns;		  // Grid: column template, kept for LVGL, which doesn't copy it
		std::vector<int32_t> m_rows;		  // Grid: row template

		static std::vector<DsLayout*> s_live; // Every ds_layout built, for find()
	};
} // namespace UI::Xml

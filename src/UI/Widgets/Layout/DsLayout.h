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
 * In a grid, each widget is in a box of its own, which the grid sizes, and fills it. The widget can't be the grid's
 * item itself: when something in it changes its content size (a label's text, a bar), LVGL forgets the size the grid
 * gave it, and a widget sized `100%` (as widgets are) would take 100% of the whole grid until the grid's next pass,
 * which in some widgets starts the same again, without end. In its box, 100% is the cell.
 *
 * The ds_layout follows its slot's layout: when it changes (see UI/Xml/Layout/LayoutStore.h) only the widgets that
 * changed are rebuilt. A widget that moved keeps its state; one whose settings changed is built again.
 *
 * While the user customises the screen (UI/Xml/Layout/LayoutEditor.h), each placed widget is covered by an edit
 * overlay, an XML component (customise_item.xml in a strip, customise_grid_item.xml in a grid) that takes its touches:
 * a tap selects the widget, and a drag moves it a cell at a time (in a grid) or along the strip. Dragging a grid
 * widget's handle (the overlay's child called `handle`) resizes it. A tap on an empty part of the ds_layout selects
 * that place to add a widget, and a grid marks the cell with customise_place.xml. Everything is done through the
 * editor, whose drafts the ds_layout then shows like any other layout.
 */

#pragma once

#include "UI/Xml/Layout/Layout.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "helix_xml.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace UI::Xml
{
	class XmlView;

	class DsLayout : public LayoutListener, public EditorListener
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

		/** Customising started or ended, or the selection changed: show the edit overlays and the selected place */
		void onEditorChanged() override;

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
			lv_obj_t* cell = nullptr;					// Grid: the box the grid sizes, holding root; else nullptr
			lv_obj_t* overlay = nullptr;				// While customising, its edit overlay (in cell, else root)

			/** What the ds_layout places: the cell in a grid, else the widget */
			lv_obj_t* outer() const { return cell ? cell : root; }
		};

		/** A drag of a placed widget, while customising */
		struct Drag
		{
			std::string id;		  // The item dragged
			lv_point_t start{};	  // Where the touch began
			LayoutItem from;	  // The item as it was then
			bool resize = false; // Resizing (the handle was touched), else moving
		};

		/** `<ds_layout>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_layout>` apply callback: lv_obj attributes, then start showing the slot's layout */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A grid cell asks how far beyond itself it draws: as far as its widget does */
		static void cellExtDrawSizeCb(lv_event_t* e);

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

		/** Give every placed widget an edit overlay while customising, and none otherwise */
		void showOverlays();

		/** Cover `placement`'s widget with an edit overlay */
		void addOverlay(Placement& placement);

		/** Take `placement`'s edit overlay away */
		static void removeOverlay(Placement& placement);

		/** Size and place the overlay `overlay` to cover its parent, padding and all */
		static void fitOverlay(lv_obj_t* overlay);

		/** The overlay's parent changed size or style: cover it again */
		static void overlayFitCb(lv_event_t* e);

		/** A touch on an edit overlay: select, move or resize its widget */
		static void overlayEventCb(lv_event_t* e);

		/** A tap on the ds_layout itself, while customising: select the place tapped */
		static void containerClickedCb(lv_event_t* e);

		/** The touch on the dragged widget moved to `point`: move or resize it to where that is in cells */
		void dragTo(const lv_point_t& point);

		/** The grid cell at `point`, or nullopt if it isn't in the grid */
		std::optional<std::pair<int32_t, int32_t>> cellAt(const lv_point_t& point) const;

		/** The width and height of a grid cell and its gap, in pixels */
		std::pair<int32_t, int32_t> cellStep() const;

		/** Mark the selected place, if it is a cell of this grid */
		void showPlace();

		/** The placement whose overlay is `overlay`, or nullptr */
		Placement* placementOf(lv_obj_t* overlay);

		lv_obj_t* m_container;				  // The ds_layout widget
		std::string m_slot;					  // The slot shown
		std::vector<Placement> m_placements;  // The placed widgets, in order
		std::vector<int32_t> m_columns;		  // Grid: column template, kept for LVGL, which doesn't copy it
		std::vector<int32_t> m_rows;		  // Grid: row template
		bool m_editing = false;				  // Whether the overlays are shown
		std::optional<Drag> m_drag;			  // The drag going on, if any
		lv_obj_t* m_place = nullptr;		  // The marker on the selected cell, or nullptr

		static std::vector<DsLayout*> s_live; // Every ds_layout built, for find()
	};
} // namespace UI::Xml

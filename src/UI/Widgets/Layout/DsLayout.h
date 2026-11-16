/*
 * DsLayout.h
 *
 * <ds_layout slot="dashboard">: shows the widgets the layout of a slot places (see UI/Xml/Layout/Layout.h). Its
 * attributes are in registerXml().
 *
 * Each placed widget is an XmlView of the widget's component, with the widget's props and settings as the component's
 * props, so a widget with a presenter has one of its own. Its names are its presenter's (`presenter.*`), its
 * PlacementScope's (`widget.*`: the size classes of its place in a free layout, see UI/Xml/Layout/PlacementScope.h),
 * and the globals. A free layout
 * puts each widget at its rectangle, scaled if the ds_layout's content area isn't the size the layout was made for; a
 * strip leaves the ds_layout's own layout (e.g. `flex_flow="column"`) to place them in order.
 *
 * In a free layout, each widget is in a box of its own, which the ds_layout sizes, and fills it: so the widget's
 * content never changes the size its place is measured by, and nothing it shows for its size can make it resize.
 *
 * The ds_layout follows its slot's layout: when it changes (see UI/Xml/Layout/LayoutStore.h) only the widgets that
 * changed are rebuilt. A widget that moved keeps its state; one whose settings changed is built again.
 *
 * While the user customises the screen (UI/Xml/Layout/LayoutEditor.h), each placed widget is covered by an edit
 * overlay, an XML component (customise_item.xml in a strip, customise_free_item.xml in a free layout) that takes its
 * touches: a tap selects the widget, and a drag moves it (along the strip, or anywhere it fits). A selected free
 * widget shows crop-tool handles on its corners and edges, and a drag that starts near an edge (within a band of 40
 * pixels, or a third of the widget across if it is small) moves that edge, and near a corner both: the bands are
 * larger than the handles drawn, so they are easy to hit, and never overlap, so the middle always moves the widget.
 * While the layout snaps, a dragged edge near another widget's edge, or the area's, snaps to it (UI/Xml/Layout/Snap.h)
 * and a guide (customise_guide.xml) shows what it snapped to. A widget that runs into another stops, sliding along it.
 * A tap on an empty part of the ds_layout selects that place to add a widget, and a free layout marks it with
 * customise_place.xml. Everything is done through the editor, whose drafts the ds_layout then shows like any other
 * layout.
 */

#pragma once

#include "UI/Xml/Layout/Layout.h"
#include "UI/Xml/Layout/LayoutEditor.h"
#include "UI/Xml/Layout/LayoutStore.h"
#include "UI/Xml/Layout/PlacementScope.h"
#include "UI/Xml/Layout/Snap.h"
#include "helix_xml.h"
#include <map>
#include <memory>
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
			std::unique_ptr<PlacementScope> names;		// Its place's size classes, searched after its presenter's names
			XmlView* view = nullptr;					// The view showing it; deleted with its widgets
			lv_obj_t* root = nullptr;					// The view's root widget; nullptr once deleted
			lv_obj_t* cell = nullptr;					// Free: the box the ds_layout places, holding root; else nullptr
			lv_obj_t* overlay = nullptr;				// While customising, its edit overlay (in its box, else root)

			/** What the ds_layout places: the box in a free layout, else the widget */
			lv_obj_t* outer() const { return cell ? cell : root; }
		};

		/** A drag of a placed widget, while customising */
		struct Drag
		{
			std::string id;	   // The item dragged
			lv_point_t start{}; // Where the touch began
			LayoutItem from;   // The item as it was then
			Snap::Edges edges; // The edges grabbed (near which it was touched); none moves it whole
		};

		/** `<ds_layout>` create callback: a plain container */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/**
		 * `<ds_layout>` apply callback: lv_obj attributes, then start showing the slot's layout. Applied again to a
		 * ds_layout already showing one (a component's `<view extends="ds_layout">` gets its instance's attributes
		 * too), it only applies the lv_obj attributes: the first apply's `slot` stays.
		 */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** A free layout's box asks how far beyond itself it draws: as far as its widget does */
		static void cellExtDrawSizeCb(lv_event_t* e);

		/** A placed widget's root widget is being deleted: forget it */
		static void viewDeletedCb(lv_event_t* e);

		/** Show the slot's current layout, rebuilding only what changed */
		void refresh();

		/** The ds_layout's content area changed size: place every widget of a free layout again, to scale */
		static void containerSizeCb(lv_event_t* e);

		/** How many screen pixels a pixel of `layout`'s area is, across and down: 1 unless the content area differs */
		std::pair<double, double> scaleOf(const Layout& layout) const;

		/** `rect` of `layout`'s area on the screen, in the ds_layout's content area */
		lv_area_t toScreen(const Layout& layout, const Rect& rect) const;

		/** The point of `layout`'s area at screen point `point`, or nullopt if it isn't in the content area */
		std::optional<Point> toLayout(const Layout& layout, const lv_point_t& point) const;

		/** Build the widget for `item` at position `index` among the placed widgets */
		Placement build(const LayoutItem& item, uint32_t index);

		/** Put a free item's box at its rectangle */
		void placeFree(lv_obj_t* obj, const Layout& layout, const LayoutItem& item) const;

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

		/** The touch on the dragged widget moved to `point`: move or resize it there, snapping if the layout does */
		void dragTo(const lv_point_t& point);

		/** Drag a free item to follow the touch at `point` */
		void dragFree(const Layout& layout, const lv_point_t& point);

		/** Show a guide for each of `guides`, and none once it is empty */
		void showGuides(const Layout& layout, const std::vector<Snap::Guide>& guides);

		/** Mark the selected place, if it is a place in this free layout */
		void showPlace();

		/** Which edges a drag from `point` on the selected widget's `overlay` moves (see the file comment) */
		static Snap::Edges grabbedEdges(lv_obj_t* overlay, const lv_point_t& point);

		/** The placement whose overlay is `overlay`, or nullptr */
		Placement* placementOf(lv_obj_t* overlay);

		lv_obj_t* m_container;				  // The ds_layout widget
		std::string m_slot;					  // The slot shown
		std::vector<Placement> m_placements;  // The placed widgets, in order
		bool m_editing = false;				  // Whether the overlays are shown
		std::optional<Drag> m_drag;			  // The drag going on, if any
		lv_obj_t* m_place = nullptr;		  // The marker on the selected place, or nullptr
		std::vector<lv_obj_t*> m_guides;	  // The guides shown while a dragged widget snaps

		static std::vector<DsLayout*> s_live; // Every ds_layout built, for find()
	};
} // namespace UI::Xml

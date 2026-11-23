/*
 * Layout.h
 *
 * A layout says which widgets (see WidgetCatalogue.h) a slot of the screen shows, and where. A slot is a
 * `<ds_layout slot="…">` in the XML; its layout is a file in a bundle's `layouts/` folder:
 *
 *     {
 *       "format": 1,
 *       "slot": "dashboard",
 *       "kind": "free",
 *       "width": 920, "height": 561,
 *       "snap": true,
 *       "items": [
 *         {"id": "tools", "widget": "tools", "x": 0, "y": 0, "w": 548, "h": 277},
 *         {"id": "fan1", "widget": "fan", "x": 553, "y": 466, "w": 367, "h": 95, "settings": {"fan": 1}}
 *       ]
 *     }
 *
 * A `free` layout places each widget in a rectangle of pixels of an area `width` by `height`, the size of the slot it
 * was made for: a slot of another size shows it scaled to fit. Rectangles may touch but not overlap. `snap` says
 * whether widgets the user drags snap to the edges near them (see Snap.h). A `strip` (a sidebar, a row of buttons)
 * shows its widgets one after another in the order listed, sized by their components. An item marked
 * `"locked": true` (the E-stop, the Settings button) can be moved but never removed or reconfigured, whatever later
 * layouts say.
 *
 * Every layout read is repaired (repair()) rather than rejected, so a hand-edited or outdated file shows as much as it
 * can: what was wrong is reported, and an item whose widget isn't installed keeps its place, as a placeholder.
 *
 * `format` is the version of this file format. A file of an older format is upgraded when read; one of a newer
 * format is rejected.
 */

#pragma once

#include "Geometry.h"
#include "WidgetCatalogue.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	/** How a layout places its widgets */
	enum class LayoutKind
	{
		Free, // In rectangles of pixels, anywhere in its area
		Strip // One after another
	};

	/** One widget placed in a layout */
	struct LayoutItem
	{
		std::string id{};							   // Unique in its layout
		std::string widget{};						   // Id of the widget shown
		int32_t x = 0;							   // Free: left edge, pixels from the area's
		int32_t y = 0;							   // Free: top edge, pixels from the area's
		int32_t w = 1;							   // Free: width, pixels
		int32_t h = 1;							   // Free: height, pixels
		nlohmann::json settings = nlohmann::json::object(); // Setting values by name, including unknown ones, kept
		bool locked = false;					   // Can't be removed or reconfigured

		/** Its rectangle in a free layout */
		Rect rect() const { return {.x = x, .y = y, .w = w, .h = h}; }

		/** Put it at `rect` in a free layout */
		void setRect(const Rect& rect);

		/** Equal if every member is */
		bool operator==(const LayoutItem&) const = default;
	};

	/** What one slot shows */
	struct Layout
	{
		/** The current file format */
		static constexpr int FORMAT = 1;

		/** The largest area a free layout can have, pixels each way */
		static constexpr int32_t MAX_AREA = 4096;

		std::string slot{};					// The slot it is for
		LayoutKind kind = LayoutKind::Free; // How it places its widgets
		int32_t width = 0;					// Free: the area's width, pixels: the slot's it was made for
		int32_t height = 0;					// Free: the area's height, pixels
		bool snap = true;					// Free: whether dragged widgets snap to the edges near them
		std::vector<LayoutItem> items{};		// The widgets, in order

		/** Equal if every member is */
		bool operator==(const Layout&) const = default;

		/** The item with this id, or nullptr */
		const LayoutItem* find(std::string_view id) const;

		/** The item with this id, or nullptr */
		LayoutItem* find(std::string_view id);

		/** The free layout's area */
		Size area() const { return {width, height}; }

		/** The rectangles of every item but `ignore`, e.g. to find room among them */
		std::vector<Rect> taken(std::string_view ignore = {}) const;

		/** An id no item has yet, made from `base`, e.g. "fan", "fan_2" */
		std::string newId(std::string_view base) const;

		/**
		 * Read a layout file. Only the structure is checked; see repair() for the contents.
		 * @param error Set to what is wrong if nullopt is returned
		 */
		static std::optional<Layout> fromJson(const nlohmann::json& json, std::string& error);

		/** The layout as a file of the current format */
		nlohmann::json toJson() const;
	};

	/** A layout file read from a bundle */
	struct LayoutFile
	{
		Layout layout{};				// As read, before repair
		std::filesystem::path file{}; // Where it was read from
		bool local = false;			// Whether it is in the local bundle (the user's own; see LayoutStore.h)
	};

	namespace Layouts
	{
		/**
		 * Make `layout` one that can be shown, reporting each change made:
		 *   - a free layout's area is limited to 1..MAX_AREA pixels each way
		 *   - items without an id, or with one used before, get a new id
		 *   - an item whose widget doesn't fit the slot is removed; one whose widget isn't installed is kept
		 *   - settings that aren't values of the widget's settings are removed (unknown ones are kept for later)
		 *   - free items are sized within their widget's limits and the area, and moved to the nearest free place if
		 *     they overlap one before them (at their smallest size if need be); if there is none, they are removed
		 *   - every locked item of `locks` is put back, as it is there, if it is missing or was changed
		 * @param locks The layout whose locked items must be kept, or nullptr
		 * @return what was changed, one message each
		 */
		std::vector<std::string> repair(Layout& layout, const Layout* locks = nullptr);
	} // namespace Layouts
} // namespace UI::Xml

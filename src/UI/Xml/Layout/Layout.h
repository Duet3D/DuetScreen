/*
 * Layout.h
 *
 * A layout says which widgets (see WidgetCatalogue.h) a slot of the screen shows, and where. A slot is a
 * `<ds_layout slot="…">` in the XML; its layout is a file in a bundle's `layouts/` folder:
 *
 *     {
 *       "format": 1,
 *       "slot": "dashboard",
 *       "kind": "grid",
 *       "columns": 10, "rows": 6,
 *       "items": [
 *         {"id": "tools", "widget": "tools", "x": 0, "y": 0, "w": 6, "h": 3},
 *         {"id": "fan1", "widget": "fan", "x": 6, "y": 4, "w": 3, "h": 2, "settings": {"fan": 1}}
 *       ]
 *     }
 *
 * A `grid` places each widget in a rectangle of cells; a `strip` (a sidebar, a row of buttons) shows its widgets one
 * after another in the order listed, sized by their components. An item marked `"locked": true` (the E-stop, the
 * Settings button) can be moved but never removed or reconfigured, whatever later layouts say.
 *
 * Every layout read is repaired (repair()) rather than rejected, so a hand-edited or outdated file shows as much as it
 * can: what was wrong is reported, and an item whose widget isn't installed keeps its place, as a placeholder.
 *
 * `format` is the version of this file format. A file of an older format is upgraded when read; one of a newer
 * format is rejected.
 */

#pragma once

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
		Grid, // In rectangles of cells
		Strip // One after another
	};

	/** One widget placed in a layout */
	struct LayoutItem
	{
		std::string id{};							   // Unique in its layout
		std::string widget{};						   // Id of the widget shown
		int32_t x = 0;							   // Grid: first column
		int32_t y = 0;							   // Grid: first row
		int32_t w = 1;							   // Grid: columns spanned
		int32_t h = 1;							   // Grid: rows spanned
		nlohmann::json settings = nlohmann::json::object(); // Setting values by name, including unknown ones, kept
		bool locked = false;					   // Can't be removed or reconfigured

		/** Equal if every member is */
		bool operator==(const LayoutItem&) const = default;
	};

	/** What one slot shows */
	struct Layout
	{
		/** The current file format */
		static constexpr int FORMAT = 1;

		/** The smallest and largest number of grid columns or rows */
		static constexpr int32_t MIN_CELLS = 1;
		static constexpr int32_t MAX_CELLS = MAX_GRID_CELLS;

		std::string slot{};					// The slot it is for
		LayoutKind kind = LayoutKind::Grid; // How it places its widgets
		int32_t columns = 10;				// Grid: columns
		int32_t rows = 6;					// Grid: rows
		std::vector<LayoutItem> items{};		// The widgets, in order

		/** Equal if every member is */
		bool operator==(const Layout&) const = default;

		/** The item with this id, or nullptr */
		const LayoutItem* find(std::string_view id) const;

		/** The item with this id, or nullptr */
		LayoutItem* find(std::string_view id);

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

	/** Which cells of a grid layout are taken */
	class GridCells
	{
	  public:
		/** Track the items of `layout`, except the one with id `ignore` */
		explicit GridCells(const Layout& layout, std::string_view ignore = {});

		/** Whether a widget can go at x, y with size w, h: inside the grid, and over no tracked item */
		bool isFree(int32_t x, int32_t y, int32_t w, int32_t h) const;

		/** Mark the cells as taken */
		void take(int32_t x, int32_t y, int32_t w, int32_t h);

		/** The first free place for a w by h widget, row by row, or nullopt if none is free */
		std::optional<std::pair<int32_t, int32_t>> findFree(int32_t w, int32_t h) const;

	  private:
		int32_t m_columns;		  // Grid width
		int32_t m_rows;			  // Grid height
		std::vector<bool> m_used{}; // Row by row, whether each cell is taken
	};

	namespace Layouts
	{
		/**
		 * Make `layout` one that can be shown, reporting each change made:
		 *   - the grid size is limited to MIN_CELLS..MAX_CELLS
		 *   - items without an id, or with one used before, get a new id
		 *   - an item whose widget doesn't fit the slot is removed; one whose widget isn't installed is kept
		 *   - settings that aren't values of the widget's settings are removed (unknown ones are kept for later)
		 *   - grid items are sized within their widget's limits and the grid, and moved to a free place if they
		 *     overlap one before them; if there is none, they are removed
		 *   - every locked item of `locks` is put back, as it is there, if it is missing or was changed
		 * @param locks The layout whose locked items must be kept, or nullptr
		 * @return what was changed, one message each
		 */
		std::vector<std::string> repair(Layout& layout, const Layout* locks = nullptr);

		/** Change a grid layout's size, moving and shrinking its items to fit; they stay in the same order */
		void resizeGrid(Layout& layout, int32_t columns, int32_t rows);
	} // namespace Layouts
} // namespace UI::Xml

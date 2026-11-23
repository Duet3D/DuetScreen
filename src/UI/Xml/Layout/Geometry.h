/*
 * Geometry.h
 *
 * Rectangles in a free layout (see Layout.h): where each widget is, in pixels of the layout's area, and finding room
 * for one. Plain values and functions, so they can be tested without a screen.
 *
 * Rectangles may touch but not overlap. Widgets placed by the UI itself (a new one, one moved out of another's way)
 * keep the theme's gap (the const `widget_gap`) from their neighbours, as the user's snapping does (Snap.h).
 */

#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace UI::Xml
{
	/** A size in pixels */
	struct Size
	{
		int32_t width = 0;	// Pixels across
		int32_t height = 0; // Pixels down

		/** Equal if both dimensions are */
		bool operator==(const Size&) const = default;
	};

	/** A point in pixels */
	struct Point
	{
		int32_t x = 0; // Pixels from the left
		int32_t y = 0; // Pixels from the top

		/** Equal if both coordinates are */
		bool operator==(const Point&) const = default;
	};

	/** A rectangle in pixels: its top left corner and its size */
	struct Rect
	{
		int32_t x = 0; // Left edge
		int32_t y = 0; // Top edge
		int32_t w = 0; // Width
		int32_t h = 0; // Height

		/** The right edge: the first pixel past the rectangle */
		int32_t right() const { return x + w; }

		/** The bottom edge: the first pixel past the rectangle */
		int32_t bottom() const { return y + h; }

		/** Whether the two share any pixel; rectangles that only touch don't */
		bool overlaps(const Rect& other) const;

		/** Whether it is all inside an area of `area` from (0, 0), and isn't empty */
		bool inside(Size area) const;

		/** Equal if every member is */
		bool operator==(const Rect&) const = default;
	};

	namespace Geometry
	{
		/** The gap the UI leaves between widgets it places, in pixels: the theme const `widget_gap`, or 0 */
		int32_t gap();

		/** Whether `rect` is inside `area` and overlaps none of `taken` */
		bool isFree(const Rect& rect, Size area, std::span<const Rect> taken);

		/**
		 * The place for a `size` rectangle in `area` that overlaps none of `taken`, nearest to `near` (its top left
		 * corner), or nullopt if there is none: `gap` pixels from every taken one if there is room for that, else
		 * touching them. Places are looked for at `near`, against the area's edges, and beside each taken rectangle,
		 * so a rectangle fits wherever there is room.
		 */
		std::optional<Rect> findFree(Size area, std::span<const Rect> taken, Size size, Point near, int32_t gap);
	} // namespace Geometry
} // namespace UI::Xml

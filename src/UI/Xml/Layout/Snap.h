/*
 * Snap.h
 *
 * Snapping a widget the user drags in a free layout (see Layout.h) to the edges near it, so widgets line up and keep
 * even gaps without the user placing them to the pixel. An edge that is moving snaps to the nearest of these within
 * DISTANCE pixels, if any:
 *
 *   - the area's edges
 *   - each other widget's edges: lined up with its same edge, touching its opposite edge, or beside it the gap away
 *     (the theme's `widget_gap`, see Geometry.h)
 *
 * A widget moved whole snaps whichever of its edges is nearest something, and keeps its size; a widget resized moves
 * only the edges dragged. Each snap is reported as a guide, a line the screen shows while dragging.
 */

#pragma once

#include "Geometry.h"
#include <span>
#include <vector>

namespace UI::Xml::Snap
{
	/** How near an edge must be to something to snap to it, in pixels */
	inline constexpr int32_t DISTANCE = 12;

	/** Which edges of a rectangle are being dragged; none moves the whole rectangle */
	struct Edges
	{
		bool left = false;	 // The left edge moves
		bool top = false;	 // The top edge moves
		bool right = false;	 // The right edge moves
		bool bottom = false; // The bottom edge moves

		/** Whether no edge is named: the whole rectangle moves */
		bool none() const { return !left && !top && !right && !bottom; }
	};

	/** A line that something snapped to: where the screen draws a guide */
	struct Guide
	{
		bool vertical = false; // A vertical line, at x = `at`; else a horizontal one, at y = `at`
		int32_t at = 0;		   // Its position across, in the area's pixels

		/** Equal if both members are */
		bool operator==(const Guide&) const = default;
	};

	/** A snapped rectangle, and what it snapped to */
	struct Result
	{
		Rect rect;				   // Where the rectangle goes
		std::vector<Guide> guides; // What it snapped to; empty if nothing was near
	};

	/**
	 * Snap `rect`, whose `edges` are being dragged, to `area`'s edges and those of `others`
	 * @param gap The gap kept between widgets, also snapped to
	 * @param minimum The smallest size a resized rectangle may snap to; a snap that would make it smaller isn't made
	 */
	Result snap(const Rect& rect, Edges edges, Size area, std::span<const Rect> others, int32_t gap, Size minimum);
} // namespace UI::Xml::Snap

/*
 * Snap.cpp
 */

#include "Snap.h"
#include <cstdlib>
#include <optional>

namespace UI::Xml::Snap
{
	namespace
	{
		/** Where the edges across one axis may snap to: an axis' start edges and its end edges have their own */
		struct Targets
		{
			std::vector<int32_t> starts; // For a left or top edge
			std::vector<int32_t> ends;	 // For a right or bottom edge
		};

		/**
		 * The targets along one axis: the area's ends; each other's same edge (to line up with it), its opposite
		 * edge (to touch it), and that edge the gap away (to sit beside it)
		 */
		Targets targets(int32_t extent, std::span<const Rect> others, bool vertical, int32_t gap)
		{
			Targets result{.starts = {0}, .ends = {extent}};
			for (const Rect& other : others)
			{
				const int32_t start = vertical ? other.x : other.y;
				const int32_t end = vertical ? other.right() : other.bottom();
				result.starts.insert(result.starts.end(), {start, end, end + gap});
				result.ends.insert(result.ends.end(), {end, start, start - gap});
			}
			return result;
		}

		/** The target nearest `edge` within DISTANCE, or nullopt */
		std::optional<int32_t> nearest(int32_t edge, const std::vector<int32_t>& targets)
		{
			std::optional<int32_t> best;
			for (const int32_t target : targets)
			{
				if (std::abs(target - edge) <= DISTANCE && (!best || std::abs(target - edge) < std::abs(*best - edge)))
				{
					best = target;
				}
			}
			return best;
		}

		/** One axis of a rectangle: where it starts and how long it is */
		struct Span
		{
			int32_t start;	// First pixel
			int32_t length; // Pixels long
		};

		/**
		 * Snap one axis of a rectangle
		 * @param moveStart, moveEnd Which ends are dragged; neither moves the whole span
		 * @param minimum The smallest length a resize may leave
		 * @param snapped Set to the positions snapped to
		 */
		Span snapAxis(Span span,
					  bool moveStart,
					  bool moveEnd,
					  const Targets& targets,
					  int32_t minimum,
					  std::vector<int32_t>& snapped)
		{
			const int32_t end = span.start + span.length;
			if (!moveStart && !moveEnd)
			{
				// Moved whole: by whichever end is nearer something
				const auto toStart = nearest(span.start, targets.starts);
				const auto toEnd = nearest(end, targets.ends);
				const bool useStart = toStart && (!toEnd || std::abs(*toStart - span.start) <= std::abs(*toEnd - end));
				if (useStart)
				{
					snapped.push_back(*toStart);
					return {*toStart, span.length};
				}
				if (toEnd)
				{
					snapped.push_back(*toEnd);
					return {*toEnd - span.length, span.length};
				}
				return span;
			}
			int32_t start = span.start;
			int32_t newEnd = end;
			if (moveStart)
			{
				if (const auto to = nearest(start, targets.starts); to && newEnd - *to >= minimum)
				{
					start = *to;
					snapped.push_back(*to);
				}
			}
			if (moveEnd)
			{
				if (const auto to = nearest(newEnd, targets.ends); to && *to - start >= minimum)
				{
					newEnd = *to;
					snapped.push_back(*to);
				}
			}
			return {start, newEnd - start};
		}
	} // namespace

	Result snap(const Rect& rect, Edges edges, Size area, std::span<const Rect> others, int32_t gap, Size minimum)
	{
		Result result{.rect = rect, .guides = {}};
		const bool whole = edges.none();
		std::vector<int32_t> xs;
		std::vector<int32_t> ys;
		// Moved whole, both axes snap; resized, only the axes with an edge dragged
		if (whole || edges.left || edges.right)
		{
			const Span span = snapAxis(
				{rect.x, rect.w}, edges.left, edges.right, targets(area.width, others, true, gap), minimum.width, xs);
			result.rect.x = span.start;
			result.rect.w = span.length;
		}
		if (whole || edges.top || edges.bottom)
		{
			const Span span = snapAxis({rect.y, rect.h},
									   edges.top,
									   edges.bottom,
									   targets(area.height, others, false, gap),
									   minimum.height,
									   ys);
			result.rect.y = span.start;
			result.rect.h = span.length;
		}
		for (const int32_t x : xs)
		{
			result.guides.push_back({.vertical = true, .at = x});
		}
		for (const int32_t y : ys)
		{
			result.guides.push_back({.vertical = false, .at = y});
		}
		return result;
	}
} // namespace UI::Xml::Snap

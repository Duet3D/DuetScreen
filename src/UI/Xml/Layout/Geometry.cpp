/*
 * Geometry.cpp
 */

#include "Geometry.h"
#include "helix_xml.h"
#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <vector>

namespace UI::Xml
{
	bool Rect::overlaps(const Rect& other) const
	{
		return x < other.right() && other.x < right() && y < other.bottom() && other.y < bottom();
	}

	bool Rect::inside(Size area) const
	{
		return w > 0 && h > 0 && x >= 0 && y >= 0 && right() <= area.width && bottom() <= area.height;
	}

	namespace Geometry
	{
		namespace
		{
			/**
			 * Where a rectangle `length` long may start along one axis: at `near`, at either end of the area, and
			 * before or after each taken rectangle, touching it or `gap` from it
			 */
			std::vector<int32_t> starts(
				int32_t near, int32_t length, int32_t extent, std::span<const Rect> taken, bool across, int32_t gap)
			{
				std::vector<int32_t> result{near, 0, extent - length};
				for (const Rect& other : taken)
				{
					const int32_t start = across ? other.x : other.y;
					const int32_t end = across ? other.right() : other.bottom();
					for (const int32_t candidate : {end, end + gap, start - length, start - length - gap, start})
					{
						result.push_back(candidate);
					}
				}
				std::erase_if(result, [&](int32_t value) { return value < 0 || value + length > extent; });
				std::sort(result.begin(), result.end());
				result.erase(std::unique(result.begin(), result.end()), result.end());
				return result;
			}
		} // namespace

		int32_t gap()
		{
			const char* text = lv_xml_get_const_silent(nullptr, "widget_gap");
			if (text == nullptr)
			{
				return 0;
			}
			const std::string_view value(text);
			int32_t pixels = 0;
			const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), pixels);
			return error == std::errc() && end == value.data() + value.size() ? std::max(0, pixels) : 0;
		}

		bool isFree(const Rect& rect, Size area, std::span<const Rect> taken)
		{
			return rect.inside(area) && std::none_of(taken.begin(),
													 taken.end(),
													 [&rect](const Rect& other) { return rect.overlaps(other); });
		}

		std::optional<Rect> findFree(Size area, std::span<const Rect> taken, Size size, Point near, int32_t gap)
		{
			// The gap from everything if there is room for that, else touching
			std::vector<Rect> spaced;
			for (const Rect& other : taken)
			{
				spaced.push_back(
					{.x = other.x - gap, .y = other.y - gap, .w = other.w + 2 * gap, .h = other.h + 2 * gap});
			}
			for (const std::span<const Rect> avoid : {std::span<const Rect>(spaced), taken})
			{
				std::optional<Rect> best;
				int64_t bestDistance = std::numeric_limits<int64_t>::max();
				for (const int32_t x : starts(near.x, size.width, area.width, taken, true, gap))
				{
					for (const int32_t y : starts(near.y, size.height, area.height, taken, false, gap))
					{
						const Rect rect{.x = x, .y = y, .w = size.width, .h = size.height};
						const int64_t dx = x - near.x;
						const int64_t dy = y - near.y;
						const int64_t distance = dx * dx + dy * dy;
						if (distance < bestDistance && isFree(rect, area, avoid))
						{
							best = rect;
							bestDistance = distance;
						}
					}
				}
				if (best)
				{
					return best;
				}
			}
			return std::nullopt;
		}
	} // namespace Geometry
} // namespace UI::Xml

/*
 * Layout.cpp
 */

#include "Layout.h"
#include <algorithm>
#include <fmt/format.h>
#include <set>

namespace UI::Xml
{
	namespace
	{
		/** The name of `kind` in a layout file */
		std::string_view kindName(LayoutKind kind)
		{
			return kind == LayoutKind::Grid ? "grid" : "strip";
		}

		/** `json[key]` as an int if it is one, else `fallback` */
		int32_t intOf(const nlohmann::json& json, const char* key, int32_t fallback)
		{
			const auto it = json.find(key);
			return it != json.end() && it->is_number_integer() ? it->get<int32_t>() : fallback;
		}

		/**
		 * Upgrade a layout file of an older format to the current one, in place. There is only format 1 so far: when
		 * the format changes, add a step here from each older format to the next.
		 * @return false (with `error` set) if the format isn't one this firmware knows
		 */
		bool upgrade(nlohmann::json& json, std::string& error)
		{
			const int format = intOf(json, "format", 0);
			if (format < 1 || format > Layout::FORMAT)
			{
				error = fmt::format("format {:d} isn't one this firmware reads (1 to {:d})", format, Layout::FORMAT);
				return false;
			}
			json["format"] = Layout::FORMAT;
			return true;
		}

		/** `value` limited to [low, high]; `high` wins if they cross */
		int32_t limit(int32_t value, int32_t low, int32_t high)
		{
			return std::min(std::max(value, low), high);
		}

		/** Scale a grid coordinate from `from` cells to `to` cells */
		int32_t scale(int32_t value, int32_t from, int32_t to)
		{
			return static_cast<int32_t>((static_cast<int64_t>(value) * to + from / 2) / from);
		}
	} // namespace

	const LayoutItem* Layout::find(std::string_view id) const
	{
		const auto it =
			std::find_if(items.begin(), items.end(), [id](const LayoutItem& item) { return item.id == id; });
		return it == items.end() ? nullptr : &*it;
	}

	LayoutItem* Layout::find(std::string_view id)
	{
		return const_cast<LayoutItem*>(std::as_const(*this).find(id));
	}

	std::string Layout::newId(std::string_view base) const
	{
		const std::string stem = base.empty() ? std::string("item") : std::string(base);
		if (find(stem) == nullptr)
		{
			return stem;
		}
		for (int n = 2;; n++)
		{
			std::string id = fmt::format("{:s}_{:d}", stem, n);
			if (find(id) == nullptr)
			{
				return id;
			}
		}
	}

	std::optional<Layout> Layout::fromJson(const nlohmann::json& source, std::string& error)
	{
		error.clear();
		if (!source.is_object())
		{
			error = "isn't a JSON object";
			return std::nullopt;
		}
		nlohmann::json json = source;
		if (!upgrade(json, error))
		{
			return std::nullopt;
		}

		Layout layout;
		const auto slot = json.find("slot");
		if (slot == json.end() || !slot->is_string() || slot->get<std::string>().empty())
		{
			error = "\"slot\" must name the slot it is for";
			return std::nullopt;
		}
		layout.slot = slot->get<std::string>();

		const std::string kind = json.value("kind", "");
		if (kind == kindName(LayoutKind::Grid))
		{
			layout.kind = LayoutKind::Grid;
		}
		else if (kind == kindName(LayoutKind::Strip))
		{
			layout.kind = LayoutKind::Strip;
		}
		else
		{
			error = "\"kind\" must be \"grid\" or \"strip\"";
			return std::nullopt;
		}
		layout.columns = intOf(json, "columns", layout.columns);
		layout.rows = intOf(json, "rows", layout.rows);

		const auto items = json.find("items");
		if (items == json.end() || !items->is_array())
		{
			error = "\"items\" must be a list";
			return std::nullopt;
		}
		for (const auto& entry : *items)
		{
			if (!entry.is_object() || !entry.contains("widget") || !entry["widget"].is_string())
			{
				error = "each item must be an object naming its \"widget\"";
				return std::nullopt;
			}
			LayoutItem item;
			item.id = entry.value("id", "");
			item.widget = entry["widget"].get<std::string>();
			item.x = intOf(entry, "x", item.x);
			item.y = intOf(entry, "y", item.y);
			item.w = intOf(entry, "w", item.w);
			item.h = intOf(entry, "h", item.h);
			item.locked = entry.value("locked", false);
			const auto settings = entry.find("settings");
			if (settings != entry.end() && settings->is_object())
			{
				item.settings = *settings;
			}
			layout.items.push_back(std::move(item));
		}
		return layout;
	}

	nlohmann::json Layout::toJson() const
	{
		nlohmann::json json = nlohmann::json::object();
		json["format"] = FORMAT;
		json["slot"] = slot;
		json["kind"] = kindName(kind);
		if (kind == LayoutKind::Grid)
		{
			json["columns"] = columns;
			json["rows"] = rows;
		}
		nlohmann::json list = nlohmann::json::array();
		for (const auto& item : items)
		{
			nlohmann::json entry = nlohmann::json::object();
			entry["id"] = item.id;
			entry["widget"] = item.widget;
			if (kind == LayoutKind::Grid)
			{
				entry["x"] = item.x;
				entry["y"] = item.y;
				entry["w"] = item.w;
				entry["h"] = item.h;
			}
			if (!item.settings.empty())
			{
				entry["settings"] = item.settings;
			}
			if (item.locked)
			{
				entry["locked"] = true;
			}
			list.push_back(std::move(entry));
		}
		json["items"] = std::move(list);
		return json;
	}

	GridCells::GridCells(const Layout& layout, std::string_view ignore)
		: m_columns(layout.columns)
		, m_rows(layout.rows)
		, m_used(static_cast<size_t>(std::max(0, layout.columns * layout.rows)), false)
	{
		for (const auto& item : layout.items)
		{
			if (item.id != ignore)
			{
				take(item.x, item.y, item.w, item.h);
			}
		}
	}

	bool GridCells::isFree(int32_t x, int32_t y, int32_t w, int32_t h) const
	{
		if (x < 0 || y < 0 || w < 1 || h < 1 || x + w > m_columns || y + h > m_rows)
		{
			return false;
		}
		for (int32_t row = y; row < y + h; row++)
		{
			for (int32_t column = x; column < x + w; column++)
			{
				if (m_used[static_cast<size_t>(row * m_columns + column)])
				{
					return false;
				}
			}
		}
		return true;
	}

	void GridCells::take(int32_t x, int32_t y, int32_t w, int32_t h)
	{
		for (int32_t row = std::max(0, y); row < std::min(m_rows, y + h); row++)
		{
			for (int32_t column = std::max(0, x); column < std::min(m_columns, x + w); column++)
			{
				m_used[static_cast<size_t>(row * m_columns + column)] = true;
			}
		}
	}

	std::optional<std::pair<int32_t, int32_t>> GridCells::findFree(int32_t w, int32_t h) const
	{
		for (int32_t y = 0; y + h <= m_rows; y++)
		{
			for (int32_t x = 0; x + w <= m_columns; x++)
			{
				if (isFree(x, y, w, h))
				{
					return std::pair{x, y};
				}
			}
		}
		return std::nullopt;
	}

	namespace Layouts
	{
		namespace
		{
			/** Put back each locked item of `locks` that `layout` lacks or has changed */
			void restoreLocks(Layout& layout, const Layout& locks, std::vector<std::string>& changes)
			{
				for (size_t i = 0; i < locks.items.size(); i++)
				{
					const LayoutItem& locked = locks.items[i];
					if (!locked.locked)
					{
						continue;
					}
					if (LayoutItem* item = layout.find(locked.id))
					{
						if (item->widget != locked.widget || item->settings != locked.settings)
						{
							changes.push_back(fmt::format("\"{:s}\" can't be changed; it is put back", locked.id));
							item->widget = locked.widget;
							item->settings = locked.settings;
						}
						item->locked = true;
						continue;
					}
					changes.push_back(fmt::format("\"{:s}\" can't be removed; it is put back", locked.id));
					// A strip keeps it where it was; a grid finds it a place below, if its own is taken
					const size_t at = std::min(i, layout.items.size());
					layout.items.insert(layout.items.begin() + static_cast<std::ptrdiff_t>(at), locked);
				}
			}

			/** Give every item an id, unique in the layout */
			void fixIds(Layout& layout, std::vector<std::string>& changes)
			{
				std::set<std::string> seen;
				for (auto& item : layout.items)
				{
					if (item.id.empty() || seen.contains(item.id))
					{
						const std::string old = item.id;
						item.id.clear();
						item.id = layout.newId(item.widget);
						if (!old.empty())
						{
							changes.push_back(
								fmt::format("two items are called \"{:s}\"; one is now \"{:s}\"", old, item.id));
						}
					}
					seen.insert(item.id);
				}
			}

			/** Remove settings that aren't values of the widget's settings */
			void fixSettings(LayoutItem& item, const WidgetDef& widget, std::vector<std::string>& changes)
			{
				if (!item.settings.is_object())
				{
					item.settings = nlohmann::json::object();
				}
				for (auto it = item.settings.begin(); it != item.settings.end();)
				{
					const SettingDef* setting = widget.setting(it.key());
					if (setting == nullptr)
					{
						++it; // Perhaps a setting of a newer version of the widget: kept for it
						continue;
					}
					if (auto value = setting->accept(it.value()))
					{
						it.value() = std::move(*value);
						++it;
						continue;
					}
					changes.push_back(
						fmt::format("\"{:s}\": {:s} isn't a value of setting \"{:s}\"; the default is used",
									item.id,
									it.value().dump(),
									it.key()));
					it = item.settings.erase(it);
				}
			}

			/** Size a grid item within its widget's limits and the grid, and inside the grid */
			void fitInGrid(LayoutItem& item, const WidgetDef* widget, const Layout& layout)
			{
				if (widget != nullptr)
				{
					item.w = limit(item.w, widget->minSize.columns, widget->maxSize.columns);
					item.h = limit(item.h, widget->minSize.rows, widget->maxSize.rows);
				}
				item.w = limit(item.w, 1, layout.columns);
				item.h = limit(item.h, 1, layout.rows);
				item.x = limit(item.x, 0, layout.columns - item.w);
				item.y = limit(item.y, 0, layout.rows - item.h);
			}

			/** Move grid items that overlap one placed before them (locked items are placed first) */
			void resolveOverlaps(Layout& layout, std::vector<std::string>& changes)
			{
				Layout empty = layout;
				empty.items.clear();
				GridCells cells(empty);

				std::vector<LayoutItem*> order;
				for (auto& item : layout.items)
				{
					order.push_back(&item);
				}
				std::stable_partition(order.begin(), order.end(), [](const LayoutItem* item) { return item->locked; });

				std::set<std::string> removed;
				for (LayoutItem* item : order)
				{
					if (cells.isFree(item->x, item->y, item->w, item->h))
					{
						cells.take(item->x, item->y, item->w, item->h);
						continue;
					}
					std::optional<std::pair<int32_t, int32_t>> place = cells.findFree(item->w, item->h);
					const WidgetDef* widget = WidgetCatalogue::find(item->widget);
					if (!place && widget != nullptr)
					{
						place = cells.findFree(widget->minSize.columns, widget->minSize.rows);
						if (place)
						{
							item->w = widget->minSize.columns;
							item->h = widget->minSize.rows;
						}
					}
					if (!place)
					{
						changes.push_back(fmt::format("\"{:s}\" doesn't fit in the grid; it is removed", item->id));
						removed.insert(item->id);
						continue;
					}
					changes.push_back(fmt::format("\"{:s}\" overlapped another widget; it is moved", item->id));
					item->x = place->first;
					item->y = place->second;
					cells.take(item->x, item->y, item->w, item->h);
				}
				std::erase_if(layout.items, [&removed](const LayoutItem& item) { return removed.contains(item.id); });
			}
		} // namespace

		std::vector<std::string> repair(Layout& layout, const Layout* locks)
		{
			std::vector<std::string> changes;
			if (layout.kind == LayoutKind::Grid)
			{
				const int32_t columns = limit(layout.columns, Layout::MIN_CELLS, Layout::MAX_CELLS);
				const int32_t rows = limit(layout.rows, Layout::MIN_CELLS, Layout::MAX_CELLS);
				if (columns != layout.columns || rows != layout.rows)
				{
					changes.push_back(
						fmt::format("a grid has {:d} to {:d} columns and rows", Layout::MIN_CELLS, Layout::MAX_CELLS));
					layout.columns = columns;
					layout.rows = rows;
				}
			}

			// Only the layout giving the locks may lock anything
			for (auto& item : layout.items)
			{
				item.locked = false;
			}
			if (locks != nullptr)
			{
				restoreLocks(layout, *locks, changes);
			}
			fixIds(layout, changes);

			std::erase_if(layout.items,
						  [&](LayoutItem& item)
						  {
							  const WidgetDef* widget = WidgetCatalogue::find(item.widget);
							  if (widget == nullptr)
							  {
								  changes.push_back(
									  fmt::format("\"{:s}\": widget \"{:s}\" isn't installed", item.id, item.widget));
								  return false;
							  }
							  if (!widget->fits(layout.slot))
							  {
								  changes.push_back(fmt::format(
									  "\"{:s}\": widget \"{:s}\" can't be placed in \"{:s}\"; it is removed",
									  item.id,
									  item.widget,
									  layout.slot));
								  return true;
							  }
							  fixSettings(item, *widget, changes);
							  return false;
						  });

			if (layout.kind == LayoutKind::Grid)
			{
				for (auto& item : layout.items)
				{
					fitInGrid(item, WidgetCatalogue::find(item.widget), layout);
				}
				resolveOverlaps(layout, changes);
			}
			return changes;
		}

		void resizeGrid(Layout& layout, int32_t columns, int32_t rows)
		{
			columns = limit(columns, Layout::MIN_CELLS, Layout::MAX_CELLS);
			rows = limit(rows, Layout::MIN_CELLS, Layout::MAX_CELLS);
			for (auto& item : layout.items)
			{
				item.x = scale(item.x, layout.columns, columns);
				item.y = scale(item.y, layout.rows, rows);
				item.w = std::max(1, scale(item.w, layout.columns, columns));
				item.h = std::max(1, scale(item.h, layout.rows, rows));
			}
			layout.columns = columns;
			layout.rows = rows;

			// Keep the locks the items have: they came from the layout giving them
			Layout locks = layout;
			std::erase_if(locks.items, [](const LayoutItem& item) { return !item.locked; });
			repair(layout, &locks);
		}
	} // namespace Layouts
} // namespace UI::Xml

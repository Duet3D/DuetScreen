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
			return kind == LayoutKind::Free ? "free" : "strip";
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
	} // namespace

	void LayoutItem::setRect(const Rect& rect)
	{
		x = rect.x;
		y = rect.y;
		w = rect.w;
		h = rect.h;
	}

	std::vector<Rect> Layout::taken(std::string_view ignore) const
	{
		std::vector<Rect> rects;
		for (const auto& item : items)
		{
			if (item.id != ignore)
			{
				rects.push_back(item.rect());
			}
		}
		return rects;
	}

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
		if (kind == kindName(LayoutKind::Free))
		{
			layout.kind = LayoutKind::Free;
			layout.width = intOf(json, "width", 0);
			layout.height = intOf(json, "height", 0);
			if (layout.width < 1 || layout.height < 1)
			{
				error = "a free layout's \"width\" and \"height\" must give its area, in pixels";
				return std::nullopt;
			}
			layout.snap = json.value("snap", true);
		}
		else if (kind == kindName(LayoutKind::Strip))
		{
			layout.kind = LayoutKind::Strip;
		}
		else
		{
			error = "\"kind\" must be \"free\" or \"strip\"";
			return std::nullopt;
		}

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
		if (kind == LayoutKind::Free)
		{
			json["width"] = width;
			json["height"] = height;
			json["snap"] = snap;
		}
		nlohmann::json list = nlohmann::json::array();
		for (const auto& item : items)
		{
			nlohmann::json entry = nlohmann::json::object();
			entry["id"] = item.id;
			entry["widget"] = item.widget;
			if (kind == LayoutKind::Free)
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
					// A strip keeps it where it was; a free layout finds it a place, if its own is taken
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

			/** Size a free item within its widget's limits and the area, and put it inside the area */
			void fitInArea(LayoutItem& item, const WidgetDef* widget, const Layout& layout)
			{
				if (widget != nullptr)
				{
					item.w = limit(item.w, widget->minSize.width, widget->maxSize.width);
					item.h = limit(item.h, widget->minSize.height, widget->maxSize.height);
				}
				item.w = limit(item.w, 1, layout.width);
				item.h = limit(item.h, 1, layout.height);
				item.x = limit(item.x, 0, layout.width - item.w);
				item.y = limit(item.y, 0, layout.height - item.h);
			}

			/** Move free items that overlap one placed before them (locked items are placed first) */
			void resolveOverlaps(Layout& layout, std::vector<std::string>& changes)
			{
				std::vector<LayoutItem*> order;
				for (auto& item : layout.items)
				{
					order.push_back(&item);
				}
				std::stable_partition(order.begin(), order.end(), [](const LayoutItem* item) { return item->locked; });

				const int32_t gap = Geometry::gap();
				std::vector<Rect> placed;
				std::set<std::string> removed;
				for (LayoutItem* item : order)
				{
					if (Geometry::isFree(item->rect(), layout.area(), placed))
					{
						placed.push_back(item->rect());
						continue;
					}
					const Point near{item->x, item->y};
					std::optional<Rect> place =
						Geometry::findFree(layout.area(), placed, {item->w, item->h}, near, gap);
					if (const WidgetDef* widget = WidgetCatalogue::find(item->widget); !place && widget != nullptr)
					{
						place = Geometry::findFree(layout.area(), placed, widget->minSize, near, gap);
					}
					if (!place)
					{
						changes.push_back(fmt::format("\"{:s}\" doesn't fit in the area; it is removed", item->id));
						removed.insert(item->id);
						continue;
					}
					changes.push_back(fmt::format("\"{:s}\" overlapped another widget; it is moved", item->id));
					item->setRect(*place);
					placed.push_back(*place);
				}
				std::erase_if(layout.items, [&removed](const LayoutItem& item) { return removed.contains(item.id); });
			}
		} // namespace

		std::vector<std::string> repair(Layout& layout, const Layout* locks)
		{
			std::vector<std::string> changes;
			if (layout.kind == LayoutKind::Free)
			{
				const int32_t width = limit(layout.width, 1, Layout::MAX_AREA);
				const int32_t height = limit(layout.height, 1, Layout::MAX_AREA);
				if (width != layout.width || height != layout.height)
				{
					changes.push_back(fmt::format("an area is 1 to {:d} pixels each way", Layout::MAX_AREA));
					layout.width = width;
					layout.height = height;
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

			if (layout.kind == LayoutKind::Free)
			{
				for (auto& item : layout.items)
				{
					fitInArea(item, WidgetCatalogue::find(item.widget), layout);
				}
				resolveOverlaps(layout, changes);
			}
			return changes;
		}
	} // namespace Layouts
} // namespace UI::Xml

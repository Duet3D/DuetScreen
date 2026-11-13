/*
 * DsList.cpp
 */

#include "DsList.h"
#include "UI/Xml/View/Actions.h"
#include "WidgetSupport.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsList::DsList(lv_obj_t* container,
				   ListBase& list,
				   std::string item,
				   lv_xml_subject_ctx_t* parentContext,
				   std::optional<Window> window)
		: m_container(container)
		, m_list(&list)
		, m_item(std::move(item))
		, m_parentContext(lv_xml_subject_ctx_ref(parentContext))
		, m_window(window)
		, m_count(list.size())
	{
		m_list->addListener(this);
		if (!m_window)
		{
			build(0, m_list->size());
			return;
		}

		// The list places the items itself
		lv_obj_set_layout(m_container, LV_LAYOUT_NONE);
		m_sizer = lv_obj_create(m_container);
		lv_obj_remove_style_all(m_sizer);
		lv_obj_remove_flag(m_sizer, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_size(m_sizer, 0, 0);
		lv_obj_add_event_cb(m_container, windowEventCb, LV_EVENT_SCROLL, this);
		lv_obj_add_event_cb(m_container, windowEventCb, LV_EVENT_SCROLL_END, this);
		lv_obj_add_event_cb(m_container, windowEventCb, LV_EVENT_SIZE_CHANGED, this);
		updateWindow();
	}

	DsList::~DsList()
	{
		if (m_list)
		{
			m_list->removeListener(this);
		}
		m_entries.clear();
		m_shown.clear();
		lv_xml_subject_ctx_unref(m_parentContext);
	}

	void DsList::registerXml()
	{
		lv_xml_register_widget("ds_list", xmlCreate, xmlApply);
	}

	void DsList::onListResized(size_t oldSize, size_t newSize)
	{
		if (m_window)
		{
			// Removed rows' widgets go now, before the rows do. The list still holds them until every listener has
			// been told, so from here on only the first newSize rows are used.
			m_count = newSize;
			for (auto it = m_shown.lower_bound(newSize); it != m_shown.end();)
			{
				deleteRow(it->second);
				it = m_shown.erase(it);
			}
			updateWindow();
		}
		else if (newSize > oldSize)
		{
			build(m_entries.size(), newSize);
		}
		else
		{
			truncate(newSize);
		}
	}

	void DsList::onListDestroyed()
	{
		truncate(0);
		for (auto& [index, entry] : m_shown)
		{
			deleteRow(entry);
		}
		m_shown.clear();
		m_list = nullptr;
	}

	void* DsList::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		return lv_obj_create(parentOf(state));
	}

	void DsList::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);

		const char* listName = lv_xml_get_value_of(attrs, "list");
		const char* item = lv_xml_get_value_of(attrs, "item");
		if (listName == nullptr || item == nullptr)
		{
			LOG_WARN("<ds_list> needs both list= and item=");
			return;
		}

		lv_xml_subject_ctx_t* context = lv_xml_subject_ctx_get_active();
		ListBase* list = Scope::findList(context, listName);
		if (list == nullptr)
		{
			LOG_WARN("<ds_list list=\"{:s}\">: no such list", listName);
			return;
		}

		std::optional<Window> window;
		if (lv_xml_to_bool(attrOr(attrs, "windowed", "false")))
		{
			window = Window{.itemWidth = std::nullopt,
							.itemHeight = lv_xml_atoi(attrOr(attrs, "item_height", "0")),
							.wrap = lv_streq(attrOr(attrs, "flow", "column"), "row_wrap")};
			if (const char* width = lv_xml_get_value_of(attrs, "item_width"))
			{
				window->itemWidth = lv_xml_atoi(width);
			}
			if (window->itemHeight <= 0)
			{
				LOG_WARN("<ds_list list=\"{:s}\" windowed=\"true\"> needs item_height=", listName);
				return;
			}
		}

		lv_obj_t* container = itemOf(state);
		deleteWith(container, new DsList(container, *list, item, context, window));
	}

	void DsList::windowEventCb(lv_event_t* e)
	{
		auto* list = static_cast<DsList*>(lv_event_get_user_data(e));
		if (list->m_list == nullptr)
		{
			return;
		}
		// While scrolling, only add items: deleting the one under the finger would stop the scroll. Items that left
		// the screen are deleted once scrolling ends.
		if (lv_event_get_code(e) == LV_EVENT_SCROLL)
		{
			list->addVisibleRows(list->itemsPerLine());
			return;
		}
		list->updateWindow();
	}

	DsList::Entry DsList::buildRow(size_t index)
	{
		ZoneScoped;
		Row& row = m_list->row(index);
		auto scope = std::make_unique<Scope>(m_parentContext);
		ScopeBinder binder(*scope);
		binder.field("index", row.index, "Position in the list");
		row.fields(binder);

		const std::string name = std::to_string(index);
		const char* attrs[] = {"name", name.c_str(), nullptr, nullptr};
		auto* obj =
			static_cast<lv_obj_t*>(lv_xml_create_with_ctx(m_container, m_item.c_str(), attrs, scope->context()));
		if (obj == nullptr)
		{
			LOG_ERROR("<ds_list>: couldn't create item component '{:s}'", m_item);
			return {nullptr, std::move(scope)};
		}

		Actions::setRow(obj, &row);
		if (m_window)
		{
			const lv_area_t area = itemArea(index, itemsPerLine());
			lv_obj_set_pos(obj, area.x1, area.y1);
			lv_obj_set_size(obj, lv_area_get_width(&area), lv_area_get_height(&area));
		}
		return {obj, std::move(scope)};
	}

	void DsList::deleteRow(Entry& entry)
	{
		if (entry.obj)
		{
			lv_obj_delete(entry.obj);
			entry.obj = nullptr;
		}
	}

	void DsList::build(size_t from, size_t to)
	{
		for (size_t i = from; i < to; i++)
		{
			m_entries.push_back(buildRow(i));
		}
	}

	void DsList::truncate(size_t count)
	{
		while (m_entries.size() > count)
		{
			deleteRow(m_entries.back());
			m_entries.pop_back();
		}
	}

	int32_t DsList::itemsPerLine() const
	{
		if (!m_window->wrap)
		{
			return 1;
		}
		const int32_t width = lv_obj_get_content_width(m_container);
		const int32_t gap = lv_obj_get_style_pad_column(m_container, LV_PART_MAIN);
		const int32_t itemWidth = m_window->itemWidth.value_or(width);
		return std::max<int32_t>(1, (width + gap) / std::max<int32_t>(1, itemWidth + gap));
	}

	lv_area_t DsList::itemArea(size_t index, int32_t perLine) const
	{
		const int32_t width = m_window->itemWidth.value_or(lv_obj_get_content_width(m_container));
		const int32_t height = m_window->itemHeight;
		const auto line = static_cast<int32_t>(index / static_cast<size_t>(perLine));
		const auto column = static_cast<int32_t>(index % static_cast<size_t>(perLine));
		const int32_t x = column * (width + lv_obj_get_style_pad_column(m_container, LV_PART_MAIN));
		const int32_t y = line * (height + lv_obj_get_style_pad_row(m_container, LV_PART_MAIN));
		return {x, y, x + width - 1, y + height - 1};
	}

	void DsList::updateWindow()
	{
		ZoneScoped;
		if (m_list == nullptr)
		{
			return;
		}

		// The scrollable area holds every row
		const size_t count = m_count;
		const int32_t perLine = itemsPerLine();
		if (count == 0)
		{
			lv_obj_set_size(m_sizer, 0, 0);
		}
		else
		{
			const lv_area_t lastItem = itemArea(count - 1, perLine);
			lv_obj_set_size(m_sizer, 1, lastItem.y2 + 1);
		}
		// A shorter list may have left the view scrolled past its end
		lv_obj_update_layout(m_container);
		lv_obj_readjust_scroll(m_container, LV_ANIM_OFF);

		// Delete the rows that left the screen; the others may need moving if the width changed
		const auto [first, last] = visibleRows(perLine);
		for (auto it = m_shown.begin(); it != m_shown.end();)
		{
			if (it->first < first || it->first >= last)
			{
				deleteRow(it->second);
				it = m_shown.erase(it);
				continue;
			}
			if (it->second.obj != nullptr)
			{
				const lv_area_t area = itemArea(it->first, perLine);
				lv_obj_set_pos(it->second.obj, area.x1, area.y1);
				lv_obj_set_size(it->second.obj, lv_area_get_width(&area), lv_area_get_height(&area));
			}
			++it;
		}
		addVisibleRows(perLine);
	}

	std::pair<size_t, size_t> DsList::visibleRows(int32_t perLine) const
	{
		const size_t count = m_count;
		const int32_t top = std::max(0, lv_obj_get_scroll_top(m_container));
		const int32_t bottom = top + lv_obj_get_content_height(m_container);
		const int32_t lineHeight = m_window->itemHeight + lv_obj_get_style_pad_row(m_container, LV_PART_MAIN);
		const auto line = [&](int32_t y) { return static_cast<size_t>(y / lineHeight) * static_cast<size_t>(perLine); };
		return {std::min(count, line(top)), std::min(count, line(bottom) + static_cast<size_t>(perLine))};
	}

	void DsList::addVisibleRows(int32_t perLine)
	{
		const auto [first, last] = visibleRows(perLine);
		for (size_t i = first; i < last; i++)
		{
			if (!m_shown.contains(i))
			{
				m_shown.emplace(i, buildRow(i));
			}
		}
	}
} // namespace UI::Xml

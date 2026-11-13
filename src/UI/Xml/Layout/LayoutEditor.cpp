/*
 * LayoutEditor.cpp
 */

#include "LayoutEditor.h"
#include "LayoutStore.h"
#include "WidgetCatalogue.h"
#include <algorithm>
#include <array>

namespace UI::Xml
{
	LayoutEditor& LayoutEditor::get()
	{
		static LayoutEditor editor;
		return editor;
	}

	void LayoutEditor::begin()
	{
		m_drafts.clear();
		for (const auto& slot : Layouts::slots())
		{
			if (const Layout* layout = Layouts::find(slot))
			{
				m_drafts.emplace(slot, *layout);
			}
		}
		m_editing = true;
		m_selection = {};
		m_undo.clear();
		m_gestureStart.reset();
		notify();
	}

	void LayoutEditor::cancel()
	{
		if (!m_editing)
		{
			return;
		}
		m_editing = false;
		for (const auto& [slot, draft] : m_drafts)
		{
			Layouts::revert(slot);
		}
		m_drafts.clear();
		m_selection = {};
		m_undo.clear();
		m_gestureStart.reset();
		notify();
	}

	bool LayoutEditor::done(std::string& error)
	{
		error.clear();
		if (!m_editing)
		{
			return true;
		}
		m_editing = false;
		bool saved = true;
		for (const auto& [slot, draft] : m_drafts)
		{
			const Layout* current = Layouts::saved(slot);
			if (current != nullptr && *current == draft)
			{
				Layouts::revert(slot);
				continue;
			}
			std::string problem;
			if (!Layouts::save(draft, problem) && saved)
			{
				error = problem;
				saved = false;
			}
		}
		m_drafts.clear();
		m_selection = {};
		m_undo.clear();
		m_gestureStart.reset();
		notify();
		return saved;
	}

	const Layout* LayoutEditor::draft(std::string_view slot) const
	{
		const auto it = m_drafts.find(slot);
		return it == m_drafts.end() ? nullptr : &it->second;
	}

	const LayoutItem* LayoutEditor::selectedItem() const
	{
		const Layout* layout = draft(m_selection.slot);
		return layout == nullptr || m_selection.item.empty() ? nullptr : layout->find(m_selection.item);
	}

	void LayoutEditor::select(const std::string& slot, const std::string& id)
	{
		const Selection selection{.slot = slot, .item = id, .cell = std::nullopt};
		if (m_editing && selection != m_selection)
		{
			m_selection = selection;
			notify();
		}
	}

	void LayoutEditor::selectPlace(const std::string& slot, std::optional<std::pair<int32_t, int32_t>> cell)
	{
		const Selection selection{.slot = slot, .item = {}, .cell = cell};
		if (m_editing && selection != m_selection)
		{
			m_selection = selection;
			notify();
		}
	}

	void LayoutEditor::clearSelection()
	{
		if (m_selection != Selection{})
		{
			m_selection = {};
			notify();
		}
	}

	bool LayoutEditor::canRemove() const
	{
		const LayoutItem* item = selectedItem();
		return item != nullptr && !item->locked;
	}

	bool LayoutEditor::canConfigure() const
	{
		const LayoutItem* item = selectedItem();
		const WidgetDef* widget = item ? WidgetCatalogue::find(item->widget) : nullptr;
		return widget != nullptr && !item->locked && !widget->settings.empty();
	}

	std::string LayoutEditor::targetSlot() const
	{
		if (!m_selection.slot.empty())
		{
			return m_selection.slot;
		}
		for (const auto& [slot, layout] : m_drafts)
		{
			if (layout.kind == LayoutKind::Grid)
			{
				return slot;
			}
		}
		return m_drafts.empty() ? std::string() : m_drafts.begin()->first;
	}

	void LayoutEditor::beginGesture()
	{
		if (m_editing)
		{
			m_gestureStart = m_drafts;
		}
	}

	void LayoutEditor::endGesture()
	{
		if (!m_gestureStart)
		{
			return;
		}
		const bool changedAnything = *m_gestureStart != m_drafts;
		if (changedAnything)
		{
			m_undo.push_back(std::move(*m_gestureStart));
		}
		m_gestureStart.reset();
		if (changedAnything)
		{
			notify();
		}
	}

	bool LayoutEditor::place(const std::string& slot, const std::string& id, int32_t x, int32_t y, int32_t w, int32_t h)
	{
		Layout* layout = editable(slot);
		LayoutItem* item = layout && layout->kind == LayoutKind::Grid ? layout->find(id) : nullptr;
		if (item == nullptr || (item->x == x && item->y == y && item->w == w && item->h == h))
		{
			return false;
		}
		if (const WidgetDef* widget = WidgetCatalogue::find(item->widget))
		{
			if (w < widget->minSize.columns || h < widget->minSize.rows || w > widget->maxSize.columns ||
				h > widget->maxSize.rows)
			{
				return false;
			}
		}
		if (!GridCells(*layout, id).isFree(x, y, w, h))
		{
			return false;
		}
		record();
		item->x = x;
		item->y = y;
		item->w = w;
		item->h = h;
		changed(slot);
		return true;
	}

	bool LayoutEditor::move(const std::string& slot, const std::string& id, int32_t x, int32_t y)
	{
		const Layout* layout = draft(slot);
		const LayoutItem* item = layout ? layout->find(id) : nullptr;
		return item != nullptr && place(slot, id, x, y, item->w, item->h);
	}

	bool LayoutEditor::resize(const std::string& slot, const std::string& id, int32_t w, int32_t h)
	{
		const Layout* layout = draft(slot);
		const LayoutItem* item = layout ? layout->find(id) : nullptr;
		return item != nullptr && place(slot, id, item->x, item->y, w, h);
	}

	bool LayoutEditor::moveTo(const std::string& slot, const std::string& id, size_t index)
	{
		Layout* layout = editable(slot);
		if (layout == nullptr || layout->kind != LayoutKind::Strip)
		{
			return false;
		}
		auto& items = layout->items;
		const auto it =
			std::find_if(items.begin(), items.end(), [&id](const LayoutItem& item) { return item.id == id; });
		if (it == items.end())
		{
			return false;
		}
		const auto from = static_cast<size_t>(it - items.begin());
		index = std::min(index, items.size() - 1);
		if (from == index)
		{
			return false;
		}
		record();
		LayoutItem moving = std::move(*it);
		items.erase(it);
		items.insert(items.begin() + static_cast<std::ptrdiff_t>(index), std::move(moving));
		changed(slot);
		return true;
	}

	std::optional<std::string> LayoutEditor::add(const std::string& widgetId)
	{
		const std::string slot = targetSlot();
		Layout* layout = editable(slot);
		const WidgetDef* widget = WidgetCatalogue::find(widgetId);
		if (layout == nullptr || widget == nullptr || !widget->fits(slot))
		{
			return std::nullopt;
		}

		LayoutItem item{.id = layout->newId(widgetId), .widget = widgetId};
		auto at = layout->items.end();
		if (layout->kind == LayoutKind::Grid)
		{
			// The selected cell, at the default size or else the smallest; else the first place either fits
			const GridCells cells(*layout);
			const std::array sizes{widget->defaultSize, widget->minSize};
			std::optional<std::pair<int32_t, int32_t>> place;
			const auto& cell = m_selection.cell;
			if (cell && m_selection.slot == slot)
			{
				for (const CellSize& size : sizes)
				{
					if (cells.isFree(cell->first, cell->second, size.columns, size.rows))
					{
						place = cell;
						item.w = size.columns;
						item.h = size.rows;
						break;
					}
				}
			}
			for (const CellSize& size : sizes)
			{
				if (place)
				{
					break;
				}
				place = cells.findFree(size.columns, size.rows);
				item.w = size.columns;
				item.h = size.rows;
			}
			if (!place)
			{
				return std::nullopt;
			}
			item.x = place->first;
			item.y = place->second;
		}
		else if (const LayoutItem* selected = selectedItem(); selected != nullptr && m_selection.slot == slot)
		{
			// After the selected item
			at = std::find_if(layout->items.begin(),
							  layout->items.end(),
							  [selected](const LayoutItem& other) { return other.id == selected->id; }) +
				 1;
		}

		record();
		const std::string id = item.id;
		layout->items.insert(at, std::move(item));
		m_selection = {.slot = slot, .item = id, .cell = std::nullopt};
		changed(slot);
		return id;
	}

	bool LayoutEditor::removeSelected()
	{
		if (!canRemove())
		{
			return false;
		}
		record();
		const std::string slot = m_selection.slot;
		std::erase_if(m_drafts.at(slot).items, [this](const LayoutItem& item) { return item.id == m_selection.item; });
		m_selection = {};
		changed(slot);
		return true;
	}

	bool LayoutEditor::setSetting(const std::string& name, const nlohmann::json& value)
	{
		if (!canConfigure())
		{
			return false;
		}
		LayoutItem* item = m_drafts.at(m_selection.slot).find(m_selection.item);
		const SettingDef* setting = WidgetCatalogue::find(item->widget)->setting(name);
		const auto accepted = setting ? setting->accept(value) : std::nullopt;
		if (!accepted)
		{
			return false;
		}
		if (item->settings.contains(name) && item->settings[name] == *accepted)
		{
			return true;
		}
		record();
		item->settings[name] = *accepted;
		changed(m_selection.slot);
		return true;
	}

	bool LayoutEditor::setGrid(const std::string& slot, int32_t columns, int32_t rows)
	{
		Layout* layout = editable(slot);
		if (layout == nullptr || layout->kind != LayoutKind::Grid)
		{
			return false;
		}
		if (layout->columns == columns && layout->rows == rows)
		{
			return true;
		}
		record();
		Layouts::resizeGrid(*layout, columns, rows);
		changed(slot);
		return true;
	}

	void LayoutEditor::reset(const std::string& slot)
	{
		Layout* layout = editable(slot);
		const Layout* fallback = Layouts::defaultFor(slot);
		if (layout == nullptr || fallback == nullptr || *layout == *fallback)
		{
			return;
		}
		record();
		*layout = *fallback;
		if (m_selection.slot == slot)
		{
			m_selection = {};
		}
		changed(slot);
	}

	void LayoutEditor::undo()
	{
		if (!m_editing || m_undo.empty())
		{
			return;
		}
		Drafts before = std::move(m_undo.back());
		m_undo.pop_back();
		std::swap(before, m_drafts);
		if (selectedItem() == nullptr && !m_selection.item.empty())
		{
			m_selection = {};
		}
		for (const auto& [slot, draft] : m_drafts)
		{
			const auto old = before.find(slot);
			if (old == before.end() || old->second != draft)
			{
				Layouts::preview(draft);
			}
		}
		notify();
	}

	void LayoutEditor::addListener(EditorListener* listener)
	{
		m_listeners.push_back(listener);
	}

	void LayoutEditor::removeListener(EditorListener* listener)
	{
		std::erase(m_listeners, listener);
	}

	Layout* LayoutEditor::editable(std::string_view slot)
	{
		if (!m_editing)
		{
			return nullptr;
		}
		const auto it = m_drafts.find(slot);
		return it == m_drafts.end() ? nullptr : &it->second;
	}

	void LayoutEditor::record()
	{
		if (!m_gestureStart)
		{
			m_undo.push_back(m_drafts);
		}
	}

	void LayoutEditor::changed(const std::string& slot)
	{
		Layouts::preview(m_drafts.at(slot));
		// The store repairs what it is shown, so the draft is what is really shown
		if (const Layout* shown = Layouts::find(slot))
		{
			m_drafts.at(slot) = *shown;
		}
		notify();
	}

	void LayoutEditor::notify()
	{
		const std::vector<EditorListener*> listeners = m_listeners;
		for (EditorListener* listener : listeners)
		{
			if (std::find(m_listeners.begin(), m_listeners.end(), listener) != m_listeners.end())
			{
				listener->onEditorChanged();
			}
		}
	}
} // namespace UI::Xml

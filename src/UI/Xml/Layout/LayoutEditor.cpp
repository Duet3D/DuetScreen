/*
 * LayoutEditor.cpp
 */

#include "LayoutEditor.h"
#include "LayoutStore.h"
#include "WidgetCatalogue.h"
#include <algorithm>

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
		const Selection selection{.slot = slot, .item = id, .point = std::nullopt};
		if (m_editing && selection != m_selection)
		{
			m_selection = selection;
			notify();
		}
	}

	void LayoutEditor::selectPlace(const std::string& slot, std::optional<Point> point)
	{
		const Selection selection{.slot = slot, .item = {}, .point = point};
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
			if (layout.kind == LayoutKind::Free)
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

	bool LayoutEditor::place(const std::string& slot, const std::string& id, const Rect& rect)
	{
		Layout* layout = editable(slot);
		LayoutItem* item = layout && layout->kind == LayoutKind::Free ? layout->find(id) : nullptr;
		if (item == nullptr || item->rect() == rect)
		{
			return false;
		}
		if (const WidgetDef* widget = WidgetCatalogue::find(item->widget))
		{
			if (rect.w < widget->minSize.width || rect.h < widget->minSize.height || rect.w > widget->maxSize.width ||
				rect.h > widget->maxSize.height)
			{
				return false;
			}
		}
		const std::vector<Rect> others = layout->taken(id);
		if (!Geometry::isFree(rect, layout->area(), others))
		{
			return false;
		}
		record();
		item->setRect(rect);
		changed(slot);
		return true;
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
		if (layout->kind == LayoutKind::Free)
		{
			// At the selected point (or the area's corner), or as near to it as there is room, at the default size or
			// else the smallest
			const Point near = m_selection.slot == slot && m_selection.point ? *m_selection.point : Point{};
			const std::vector<Rect> taken = layout->taken();
			std::optional<Rect> place;
			for (const Size& size : {widget->defaultSize, widget->minSize})
			{
				const Size fitted{std::min(size.width, layout->width), std::min(size.height, layout->height)};
				place = Geometry::findFree(layout->area(), taken, fitted, near, Geometry::gap());
				if (place)
				{
					break;
				}
			}
			if (!place || place->w < widget->minSize.width || place->h < widget->minSize.height)
			{
				return std::nullopt;
			}
			item.setRect(*place);
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
		m_selection = {.slot = slot, .item = id, .point = std::nullopt};
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

	bool LayoutEditor::setSnap(const std::string& slot, bool snap)
	{
		Layout* layout = editable(slot);
		if (layout == nullptr || layout->kind != LayoutKind::Free)
		{
			return false;
		}
		if (layout->snap != snap)
		{
			record();
			layout->snap = snap;
			changed(slot);
		}
		return true;
	}

	size_t LayoutEditor::apply(const std::vector<Layout>& layouts)
	{
		std::vector<const Layout*> usable;
		for (const auto& layout : layouts)
		{
			const Layout* current = draft(layout.slot);
			if (current != nullptr && current->kind == layout.kind && *current != layout)
			{
				usable.push_back(&layout);
			}
		}
		if (usable.empty())
		{
			return 0;
		}
		record();
		for (const Layout* layout : usable)
		{
			m_drafts.at(layout->slot) = *layout;
			if (m_selection.slot == layout->slot)
			{
				m_selection = {};
			}
			// Repaired as it is shown: locked items are put back, and what doesn't fit is moved
			changed(layout->slot);
		}
		return usable.size();
	}

	std::vector<Layout> LayoutEditor::drafts() const
	{
		std::vector<Layout> result;
		for (const auto& [slot, layout] : m_drafts)
		{
			result.push_back(layout);
		}
		return result;
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

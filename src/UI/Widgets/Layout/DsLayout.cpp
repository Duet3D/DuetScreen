/*
 * DsLayout.cpp
 */

#include "DsLayout.h"
#include "DsView.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
#include "UI/Xml/Runtime/XmlUi.h"
#include "UI/Xml/View/XmlView.h"
#include "src/xml/parsers/lv_xml_obj_parser.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The component that stands in for a widget that isn't installed */
		constexpr const char* MISSING_COMPONENT = "customise_missing";

		/** The edit overlays: over a strip's widget, and over a grid's, which has a handle to resize it */
		constexpr const char* STRIP_OVERLAY = "customise_item";
		constexpr const char* GRID_OVERLAY = "customise_grid_item";

		/** The marker on the grid cell selected to add a widget to */
		constexpr const char* PLACE_MARKER = "customise_place";

		/** `delta` pixels in whole steps of `step` pixels, to the nearest */
		int32_t steps(int32_t delta, int32_t step)
		{
			return step > 0 ? static_cast<int32_t>(std::lround(static_cast<double>(delta) / step)) : 0;
		}

		/** Whether `point` is in `area`, edges included */
		bool contains(const lv_area_t& area, const lv_point_t& point)
		{
			return point.x >= area.x1 && point.x <= area.x2 && point.y >= area.y1 && point.y <= area.y2;
		}

		/** How far in from a selected grid widget's edge a touch grabs that edge, in pixels, at most */
		constexpr int32_t GRAB_BAND = 40;

		/** Where the touch now is */
		lv_point_t touchPoint()
		{
			lv_point_t point{};
			if (lv_indev_t* indev = lv_indev_active())
			{
				lv_indev_get_point(indev, &point);
			}
			return point;
		}
	} // namespace

	std::vector<DsLayout*> DsLayout::s_live;

	DsLayout::DsLayout(lv_obj_t* container, std::string slot)
		: m_container(container)
		, m_slot(std::move(slot))
	{
		s_live.push_back(this);
		Layouts::addListener(this);
		LayoutEditor::get().addListener(this);
		lv_obj_add_event_cb(m_container, containerClickedCb, LV_EVENT_CLICKED, this);
		refresh();
		onEditorChanged(); // It may be built while the user is customising
	}

	DsLayout::~DsLayout()
	{
		std::erase(s_live, this);
		Layouts::removeListener(this);
		LayoutEditor::get().removeListener(this);
		// The views are deleted with their widgets, which LVGL deletes next
		for (auto& placement : m_placements)
		{
			if (placement.root)
			{
				lv_obj_remove_event_cb_with_user_data(placement.root, viewDeletedCb, this);
			}
			if (placement.overlay)
			{
				lv_obj_remove_event_cb_with_user_data(
					lv_obj_get_parent(placement.overlay), overlayFitCb, placement.overlay);
			}
		}
	}

	void DsLayout::registerXml()
	{
		Widgets::add(
			{.name = "ds_layout",
			 .extends = "lv_obj",
			 .description =
				 "Shows the widgets the layout of a slot places, which the user can change on the screen. The layout "
				 "comes from the `layouts/` of the bundles (see docs/XML_UI.md, Layouts). A grid layout makes this a "
				 "grid of equal cells; a strip leaves its own layout (e.g. `flex_flow`) to place the widgets in order.",
			 .example = R"(<ds_layout slot="sidebar" flex_flow="column" width="content" height="100%"/>)",
			 .exampleHeight =
				 584, // About the screen's height, as the sidebar is, so every button has room for its label
			 .attributes = {{"slot", "The slot whose layout is shown (required), e.g. `dashboard`"}}},
			xmlCreate,
			xmlApply);
	}

	void* DsLayout::xmlCreate(lv_xml_parser_state_t* state, const char** /* attrs */)
	{
		return lv_obj_create(parentOf(state));
	}

	void DsLayout::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_obj_apply(state, attrs);
		const char* slot = lv_xml_get_value_of(attrs, "slot");
		if (slot == nullptr || slot[0] == '\0')
		{
			LOG_WARN("<ds_layout> needs a slot=");
			return;
		}
		lv_obj_t* container = itemOf(state);
		deleteWith(container, new DsLayout(container, slot));
	}

	void DsLayout::onLayoutChanged(std::string_view slot)
	{
		if (slot == m_slot)
		{
			refresh();
		}
	}

	lv_obj_t* DsLayout::findItem(std::string_view id) const
	{
		const auto it = std::find_if(m_placements.begin(),
									 m_placements.end(),
									 [id](const Placement& placement) { return placement.item.id == id; });
		return it == m_placements.end() ? nullptr : it->root;
	}

	DsLayout* DsLayout::find(std::string_view slot)
	{
		const auto it = std::find_if(
			s_live.begin(), s_live.end(), [slot](const DsLayout* layout) { return layout->m_slot == slot; });
		return it == s_live.end() ? nullptr : *it;
	}

	void DsLayout::viewDeletedCb(lv_event_t* e)
	{
		auto* layout = static_cast<DsLayout*>(lv_event_get_user_data(e));
		auto* root = static_cast<lv_obj_t*>(lv_event_get_target(e));
		// Runs before the view is deleted, so the placement never points at a deleted view
		for (auto& placement : layout->m_placements)
		{
			if (placement.root == root)
			{
				placement.view = nullptr;
				placement.root = nullptr;
			}
		}
	}

	void DsLayout::refresh()
	{
		ZoneScoped;
		const Layout* layout = Layouts::find(m_slot);
		if (layout == nullptr)
		{
			LOG_WARN("<ds_layout slot=\"{:s}\">: no bundle has a layout for this slot", m_slot);
		}
		const std::vector<LayoutItem> items = layout ? layout->items : std::vector<LayoutItem>{};
		if (layout && layout->kind == LayoutKind::Grid)
		{
			applyGrid(*layout);
		}

		// Keep the widgets that are still the same, in the new order; build the rest
		std::vector<Placement> old = std::move(m_placements);
		m_placements.clear();
		for (const auto& item : items)
		{
			const WidgetDef* widget = WidgetCatalogue::find(item.widget);
			const std::string component = widget ? widget->component : MISSING_COMPONENT;
			const auto props =
				widget ? widget->propsFor(item.settings) : std::map<std::string, std::string>{{"widget", item.widget}};
			const auto index = static_cast<uint32_t>(m_placements.size());

			auto same = std::find_if(old.begin(),
									 old.end(),
									 [&](const Placement& placement)
									 { return placement.item.id == item.id && placement.root != nullptr; });
			if (same != old.end() && same->component == component && same->props == props)
			{
				Placement kept = std::move(*same);
				old.erase(same);
				kept.item = item;
				lv_obj_move_to_index(kept.outer(), static_cast<int32_t>(index));
				m_placements.push_back(std::move(kept));
			}
			else
			{
				if (same != old.end())
				{
					remove(*same);
					old.erase(same);
				}
				m_placements.push_back(build(item, index));
			}

			if (layout->kind == LayoutKind::Grid && m_placements.back().outer())
			{
				placeInGrid(m_placements.back().outer(), item);
			}
		}
		for (auto& placement : old)
		{
			remove(placement);
		}
		showOverlays(); // The widgets built again need theirs
	}

	void DsLayout::applyGrid(const Layout& layout)
	{
		const auto columns = static_cast<size_t>(layout.columns);
		const auto rows = static_cast<size_t>(layout.rows);
		if (m_columns.size() == columns + 1 && m_rows.size() == rows + 1)
		{
			return;
		}
		m_columns.assign(columns, LV_GRID_FR(1));
		m_columns.push_back(LV_GRID_TEMPLATE_LAST);
		m_rows.assign(rows, LV_GRID_FR(1));
		m_rows.push_back(LV_GRID_TEMPLATE_LAST);
		lv_obj_set_layout(m_container, LV_LAYOUT_GRID);
		lv_obj_set_grid_dsc_array(m_container, m_columns.data(), m_rows.data());
	}

	DsLayout::Placement DsLayout::build(const LayoutItem& item, uint32_t index)
	{
		ZoneScoped;
		const WidgetDef* widget = WidgetCatalogue::find(item.widget);
		Placement placement{.item = item,
							.component = widget ? widget->component : MISSING_COMPONENT,
							.props = widget ? widget->propsFor(item.settings)
											: std::map<std::string, std::string>{{"widget", item.widget}}};

		// In a grid, a plain box the grid sizes, which the widget fills (see the file comment)
		const Layout* layout = Layouts::find(m_slot);
		lv_obj_t* parent = m_container;
		if (layout != nullptr && layout->kind == LayoutKind::Grid)
		{
			placement.cell = lv_obj_create(m_container);
			lv_obj_remove_style_all(placement.cell);
			lv_obj_remove_flag(placement.cell, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_remove_flag(placement.cell, LV_OBJ_FLAG_SCROLLABLE);
			// The widget's shadow or glow shows beyond the cell, as it would without one
			lv_obj_add_flag(placement.cell, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
			lv_obj_add_event_cb(placement.cell, cellExtDrawSizeCb, LV_EVENT_REFR_EXT_DRAW_SIZE, nullptr);
			parent = placement.cell;
		}

		placement.view = new XmlView(item.id, parent, placement.component, nullptr, placement.props);
		placement.root = placement.view->getRootPtr();
		if (placement.cell != nullptr)
		{
			lv_obj_set_size(placement.root, LV_PCT(100), LV_PCT(100));
			lv_obj_refresh_ext_draw_size(placement.cell);
		}
		// Forget the placement first, then delete the view
		lv_obj_add_event_cb(placement.root, viewDeletedCb, LV_EVENT_DELETE, this);
		deleteWith(placement.root, placement.view);
		lv_obj_move_to_index(placement.outer(), static_cast<int32_t>(index));
		DsView::showWithParent(*placement.view, m_container);
		return placement;
	}

	void DsLayout::cellExtDrawSizeCb(lv_event_t* e)
	{
		auto* cell = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		if (lv_obj_get_child_count(cell) > 0)
		{
			lv_event_set_ext_draw_size(e, lv_obj_calculate_ext_draw_size(lv_obj_get_child(cell, 0), LV_PART_MAIN));
		}
	}

	void DsLayout::onEditorChanged()
	{
		const bool editing = LayoutEditor::get().isEditing();
		if (editing != m_editing)
		{
			m_editing = editing;
			m_drag.reset();
			showOverlays();
		}
		showPlace();
	}

	void DsLayout::showOverlays()
	{
		for (auto& placement : m_placements)
		{
			if (m_editing && placement.overlay == nullptr && placement.root != nullptr)
			{
				addOverlay(placement);
			}
			else if (!m_editing && placement.overlay != nullptr)
			{
				removeOverlay(placement);
			}
		}
	}

	void DsLayout::addOverlay(Placement& placement)
	{
		const Layout* layout = Layouts::find(m_slot);
		const bool grid = layout != nullptr && layout->kind == LayoutKind::Grid;
		const std::string key = fmt::format("{:s}/{:s}", m_slot, placement.item.id);
		const char* attrs[] = {"name", "customise_overlay", "key", key.c_str(), nullptr, nullptr};
		lv_obj_t* parent = placement.cell ? placement.cell : placement.root;
		auto* overlay = static_cast<lv_obj_t*>(lv_xml_create(parent, grid ? GRID_OVERLAY : STRIP_OVERLAY, attrs));
		if (overlay == nullptr)
		{
			LOG_ERROR("<ds_layout slot=\"{:s}\">: no edit overlay component", m_slot);
			return;
		}
		// Over the whole widget, in front, taking every touch: nothing scrolls, the widget underneath gets nothing
		lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING);
		lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(overlay, LV_OBJ_FLAG_PRESS_LOCK);
		lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLL_CHAIN);
		lv_obj_remove_flag(overlay, LV_OBJ_FLAG_GESTURE_BUBBLE);
		lv_obj_add_event_cb(overlay, overlayEventCb, LV_EVENT_ALL, this);
		lv_obj_add_event_cb(parent, overlayFitCb, LV_EVENT_SIZE_CHANGED, overlay);
		lv_obj_add_event_cb(parent, overlayFitCb, LV_EVENT_STYLE_CHANGED, overlay);
		placement.overlay = overlay;
		fitOverlay(overlay);
	}

	void DsLayout::removeOverlay(Placement& placement)
	{
		lv_obj_remove_event_cb_with_user_data(lv_obj_get_parent(placement.overlay), overlayFitCb, placement.overlay);
		lv_obj_delete(placement.overlay);
		placement.overlay = nullptr;
	}

	void DsLayout::fitOverlay(lv_obj_t* overlay)
	{
		// Positions are from the parent's content area, inside its border and padding
		lv_obj_t* parent = lv_obj_get_parent(overlay);
		const int32_t border = lv_obj_get_style_border_width(parent, LV_PART_MAIN);
		const int32_t x = -(lv_obj_get_style_pad_left(parent, LV_PART_MAIN) + border);
		const int32_t y = -(lv_obj_get_style_pad_top(parent, LV_PART_MAIN) + border);
		const int32_t width = lv_obj_get_width(parent);
		const int32_t height = lv_obj_get_height(parent);
		// Only a change: setting them lays the parent out again, which would call this again, without end
		if (lv_obj_get_style_x(overlay, LV_PART_MAIN) != x || lv_obj_get_style_y(overlay, LV_PART_MAIN) != y)
		{
			lv_obj_set_pos(overlay, x, y);
		}
		if (lv_obj_get_style_width(overlay, LV_PART_MAIN) != width ||
			lv_obj_get_style_height(overlay, LV_PART_MAIN) != height)
		{
			lv_obj_set_size(overlay, width, height);
		}
	}

	void DsLayout::overlayFitCb(lv_event_t* e)
	{
		fitOverlay(static_cast<lv_obj_t*>(lv_event_get_user_data(e)));
	}

	void DsLayout::overlayEventCb(lv_event_t* e)
	{
		auto* layout = static_cast<DsLayout*>(lv_event_get_user_data(e));
		auto* overlay = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		LayoutEditor& editor = LayoutEditor::get();
		switch (lv_event_get_code(e))
		{
		case LV_EVENT_PRESSED:
		{
			const Placement* placement = layout->placementOf(overlay);
			const Layout* draft = editor.draft(layout->m_slot);
			const LayoutItem* item = placement && draft ? draft->find(placement->item.id) : nullptr;
			if (item == nullptr)
			{
				return;
			}
			// A grid widget already selected (showing its handles) is resized from near its edges; a first touch
			// only selects it, and moves it if dragged
			const lv_point_t point = touchPoint();
			const LayoutEditor::Selection& selection = editor.selection();
			const bool wasSelected = selection.slot == layout->m_slot && selection.item == item->id;
			editor.select(layout->m_slot, item->id);
			layout->m_drag = Drag{.id = item->id, .start = point, .from = *item};
			if (wasSelected && draft->kind == LayoutKind::Grid)
			{
				grabbedEdges(overlay, point, *layout->m_drag);
			}
			editor.beginGesture();
			break;
		}
		case LV_EVENT_PRESSING:
			if (layout->m_drag)
			{
				layout->dragTo(touchPoint());
			}
			break;
		case LV_EVENT_RELEASED:
		case LV_EVENT_PRESS_LOST:
			if (layout->m_drag)
			{
				layout->m_drag.reset();
				editor.endGesture();
			}
			break;
		default:
			break;
		}
	}

	void DsLayout::containerClickedCb(lv_event_t* e)
	{
		auto* layout = static_cast<DsLayout*>(lv_event_get_user_data(e));
		LayoutEditor& editor = LayoutEditor::get();
		const Layout* draft = editor.draft(layout->m_slot);
		if (!editor.isEditing() || draft == nullptr)
		{
			return;
		}
		if (draft->kind == LayoutKind::Grid)
		{
			editor.selectPlace(layout->m_slot, layout->cellAt(touchPoint()));
		}
		else
		{
			editor.selectPlace(layout->m_slot);
		}
	}

	void DsLayout::dragTo(const lv_point_t& point)
	{
		LayoutEditor& editor = LayoutEditor::get();
		const Layout* draft = editor.draft(m_slot);
		if (draft == nullptr)
		{
			return;
		}
		const Drag& drag = *m_drag;
		if (draft->kind == LayoutKind::Grid)
		{
			const auto [stepX, stepY] = cellStep();
			const int32_t columns = steps(point.x - drag.start.x, stepX);
			const int32_t rows = steps(point.y - drag.start.y, stepY);
			if (!drag.left && !drag.top && !drag.right && !drag.bottom)
			{
				editor.move(m_slot, drag.id, drag.from.x + columns, drag.from.y + rows);
				return;
			}
			// The edges dragged follow the touch; the others stay where they were
			const LayoutItem& from = drag.from;
			const int32_t x = drag.left ? from.x + columns : from.x;
			const int32_t y = drag.top ? from.y + rows : from.y;
			const int32_t right = drag.right ? from.x + from.w + columns : from.x + from.w;
			const int32_t bottom = drag.bottom ? from.y + from.h + rows : from.y + from.h;
			editor.place(m_slot, drag.id, x, y, right - x, bottom - y);
			return;
		}

		// Along the strip: before every other widget whose middle the touch is past
		const bool column = lv_obj_get_style_flex_flow(m_container, LV_PART_MAIN) == LV_FLEX_FLOW_COLUMN;
		size_t index = 0;
		for (const auto& placement : m_placements)
		{
			if (placement.item.id == drag.id || placement.root == nullptr)
			{
				continue;
			}
			lv_area_t area;
			lv_obj_get_coords(placement.root, &area);
			const int32_t middle = column ? (area.y1 + area.y2) / 2 : (area.x1 + area.x2) / 2;
			if ((column ? point.y : point.x) > middle)
			{
				index++;
			}
		}
		editor.moveTo(m_slot, drag.id, index);
	}

	std::pair<int32_t, int32_t> DsLayout::cellStep() const
	{
		const Layout* layout = Layouts::find(m_slot);
		if (layout == nullptr || layout->kind != LayoutKind::Grid)
		{
			return {0, 0};
		}
		const int32_t gapX = lv_obj_get_style_pad_column(m_container, LV_PART_MAIN);
		const int32_t gapY = lv_obj_get_style_pad_row(m_container, LV_PART_MAIN);
		return {(lv_obj_get_content_width(m_container) + gapX) / layout->columns,
				(lv_obj_get_content_height(m_container) + gapY) / layout->rows};
	}

	std::optional<std::pair<int32_t, int32_t>> DsLayout::cellAt(const lv_point_t& point) const
	{
		const Layout* layout = Layouts::find(m_slot);
		const auto [stepX, stepY] = cellStep();
		lv_area_t content;
		lv_obj_get_content_coords(m_container, &content);
		if (layout == nullptr || stepX <= 0 || stepY <= 0 || !contains(content, point))
		{
			return std::nullopt;
		}
		return std::pair{std::min((point.x - content.x1) / stepX, layout->columns - 1),
						 std::min((point.y - content.y1) / stepY, layout->rows - 1)};
	}

	void DsLayout::showPlace()
	{
		const LayoutEditor& editor = LayoutEditor::get();
		const LayoutEditor::Selection& selection = editor.selection();
		const Layout* layout = Layouts::find(m_slot);
		const bool marked = m_editing && layout != nullptr && layout->kind == LayoutKind::Grid &&
							selection.slot == m_slot && selection.item.empty() && selection.cell.has_value();
		if (!marked)
		{
			if (m_place != nullptr)
			{
				lv_obj_delete(m_place);
				m_place = nullptr;
			}
			return;
		}
		if (m_place == nullptr)
		{
			const char* attrs[] = {"name", "customise_place", nullptr, nullptr};
			m_place = static_cast<lv_obj_t*>(lv_xml_create(m_container, PLACE_MARKER, attrs));
			if (m_place == nullptr)
			{
				LOG_ERROR("<ds_layout slot=\"{:s}\">: no place marker component", m_slot);
				return;
			}
		}
		lv_obj_set_grid_cell(
			m_place, LV_GRID_ALIGN_STRETCH, selection.cell->first, 1, LV_GRID_ALIGN_STRETCH, selection.cell->second, 1);
	}

	void DsLayout::grabbedEdges(lv_obj_t* overlay, const lv_point_t& point, Drag& drag)
	{
		// A band along each edge, at most a third of the widget across, so the bands never meet and the middle
		// always moves the widget, however small it is
		lv_area_t area;
		lv_obj_get_coords(overlay, &area);
		const int32_t bandX = std::min(GRAB_BAND, lv_area_get_width(&area) / 3);
		const int32_t bandY = std::min(GRAB_BAND, lv_area_get_height(&area) / 3);
		drag.left = point.x < area.x1 + bandX;
		drag.right = point.x > area.x2 - bandX;
		drag.top = point.y < area.y1 + bandY;
		drag.bottom = point.y > area.y2 - bandY;
	}

	DsLayout::Placement* DsLayout::placementOf(lv_obj_t* overlay)
	{
		const auto it = std::find_if(m_placements.begin(),
									 m_placements.end(),
									 [overlay](const Placement& placement) { return placement.overlay == overlay; });
		return it == m_placements.end() ? nullptr : &*it;
	}

	void DsLayout::placeInGrid(lv_obj_t* obj, const LayoutItem& item)
	{
		lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, item.x, item.w, LV_GRID_ALIGN_STRETCH, item.y, item.h);
	}

	void DsLayout::remove(Placement& placement)
	{
		lv_obj_t* outer = placement.outer();
		placement.view = nullptr;
		placement.root = nullptr;
		placement.cell = nullptr;
		placement.overlay = nullptr;
		if (outer != nullptr)
		{
			lv_obj_delete(outer);
		}
	}
} // namespace UI::Xml

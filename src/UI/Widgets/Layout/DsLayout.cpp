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

namespace UI::Xml
{
	using namespace WidgetSupport;

	namespace
	{
		/** The component that stands in for a widget that isn't installed */
		constexpr const char* MISSING_COMPONENT = "customise_missing";
	} // namespace

	DsLayout::DsLayout(lv_obj_t* container, std::string slot)
		: m_container(container)
		, m_slot(std::move(slot))
	{
		Layouts::addListener(this);
		refresh();
	}

	DsLayout::~DsLayout()
	{
		Layouts::removeListener(this);
		// The views are deleted with their widgets, which LVGL deletes next
		for (auto& placement : m_placements)
		{
			if (placement.root)
			{
				lv_obj_remove_event_cb_with_user_data(placement.root, viewDeletedCb, this);
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
				lv_obj_move_to_index(kept.root, static_cast<int32_t>(index));
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

			if (layout->kind == LayoutKind::Grid && m_placements.back().root)
			{
				placeInGrid(m_placements.back().root, item);
			}
		}
		for (auto& placement : old)
		{
			remove(placement);
		}
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

		placement.view = new XmlView(item.id, m_container, placement.component, nullptr, placement.props);
		placement.root = placement.view->getRootPtr();
		// Forget the placement first, then delete the view
		lv_obj_add_event_cb(placement.root, viewDeletedCb, LV_EVENT_DELETE, this);
		deleteWith(placement.root, placement.view);
		lv_obj_move_to_index(placement.root, static_cast<int32_t>(index));
		DsView::showWithParent(*placement.view, m_container);
		return placement;
	}

	void DsLayout::placeInGrid(lv_obj_t* obj, const LayoutItem& item)
	{
		lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, item.x, item.w, LV_GRID_ALIGN_STRETCH, item.y, item.h);
	}

	void DsLayout::remove(Placement& placement)
	{
		if (placement.root)
		{
			lv_obj_t* root = placement.root;
			placement.view = nullptr;
			placement.root = nullptr;
			lv_obj_delete(root);
		}
	}
} // namespace UI::Xml

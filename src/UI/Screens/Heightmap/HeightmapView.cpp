#include "HeightmapView.h"
#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "lv_i18n/lv_i18n.h"

#include "ObjectModel/Heightmap.h"
#include <cmath>

namespace UI
{
	class HeightmapItem : public ListItem
	{
	  public:
		HeightmapItem(size_t index, LvObj& parent, HeightmapView& view)
			: ListItem(index, parent)
			, m_view(view)
			, m_label("label", getRoot())
			, m_load("load", getRoot(), "", layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
		{
			UI_LOCK();

			setFlexFlow(LV_FLEX_FLOW_ROW);
			setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			setSize(LV_PCT(100), LV_SIZE_CONTENT);
			m_label.setHeight(LV_SIZE_CONTENT);
			m_label.setFlexGrow(1);

			setFlag(LV_OBJ_FLAG_CLICKABLE, true);
			addEventCallback(
				[](lv_event_t* event)
				{
					UI_LOCK();
					auto item = static_cast<HeightmapItem*>(lv_event_get_user_data(event));
					if (item == nullptr)
					{
						LOG_WARN("Heightmap item is null");
						return;
					}
					item->m_view.getPresenter()->setActiveHeightmap(item->getIndex());
				},
				LV_EVENT_CLICKED,
				this);

			m_load.addClickedCallback(
				[](lv_event_t* event)
				{
					UI_LOCK();
					auto item = static_cast<HeightmapItem*>(lv_event_get_user_data(event));
					if (item == nullptr)
					{
						LOG_WARN("Heightmap item is null");
						return;
					}
					item->m_view.getPresenter()->toggleHeightmap(item->getIndex());
					item->m_view.getPresenter()->setActiveHeightmap(item->getIndex());
				},
				this);
			// m_load.setCheckable(true);

			m_load.setSize(70, LV_SIZE_CONTENT);
			addStyle(Themes::getLvglStyles().bg_color_primary, LV_STATE_CHECKED);
			m_load.addStyle(Themes::getLvglStyles().actionBtn, 0);
		}

		void setLabel(std::string_view label)
		{
			UI_LOCK();
			m_label.setText(label);
		}

		void setSelected(bool selected)
		{
			UI_LOCK();
			lv_obj_set_state(getRoot(), LV_STATE_CHECKED, selected);
			m_load.setText(selected ? _("heightmap_unload") : _("heightmap_load"));
			m_load.setChecked(selected);
		}

	  private:
		HeightmapView& m_view;

		LvLabel m_label;
		Button m_load;
	};

	HeightmapRenderMode::HeightmapRenderMode(LvObj& parent, HeightmapPresenter& presenter)
		: LvContainer("heightmap_render_mode", parent)
		, m_presenter(presenter)
		, m_title("label", getRoot())
		, m_btns("btns", getRoot())
		, m_fixed("heightmap_fixed", m_btns, _("heightmap_fixed"), layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
		, m_auto("heightmap_auto", m_btns, _("heightmap_auto"), layout_t(0, 0, LV_SIZE_CONTENT, LV_SIZE_CONTENT))
	{
		UI_LOCK();
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_title.setText(_("heightmap_render_mode"));

		m_btns.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_btns.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_btns.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_fixed.setHeight(LV_SIZE_CONTENT);
		m_auto.setHeight(LV_SIZE_CONTENT);

		m_fixed.setFlexGrow(1);
		m_auto.setFlexGrow(1);

		setRenderMode(m_presenter.getRenderMode());

		m_fixed.addClickedCallback(
			[](lv_event_t* event)
			{
				UI_LOCK();
				auto mode = static_cast<HeightmapRenderMode*>(lv_event_get_user_data(event));
				if (mode == nullptr)
				{
					LOG_WARN("Heightmap render mode is null");
					return;
				}
				mode->m_presenter.setRenderMode(HeightmapPresenter::HeightmapRenderMode::Fixed);
				mode->setRenderMode(HeightmapPresenter::HeightmapRenderMode::Fixed);
			},
			this);

		m_auto.addClickedCallback(
			[](lv_event_t* event)
			{
				UI_LOCK();
				auto mode = static_cast<HeightmapRenderMode*>(lv_event_get_user_data(event));
				if (mode == nullptr)
				{
					LOG_WARN("Heightmap render mode is null");
					return;
				}
				mode->m_presenter.setRenderMode(HeightmapPresenter::HeightmapRenderMode::Auto);
				mode->setRenderMode(HeightmapPresenter::HeightmapRenderMode::Auto);
			},
			this);
	}

	void HeightmapRenderMode::setRenderMode(HeightmapPresenter::HeightmapRenderMode mode)
	{
		UI_LOCK();
		switch (mode)
		{
		case HeightmapPresenter::HeightmapRenderMode::Fixed:
			m_fixed.setChecked(true);
			m_auto.setChecked(false);
			break;
		case HeightmapPresenter::HeightmapRenderMode::Auto:
			m_fixed.setChecked(false);
			m_auto.setChecked(true);
			break;
		default:
			break;
		}
	}

	HeightmapStatistics::HeightmapStatistics(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
		, m_numPoints("num_points", getRoot())
		, m_area("area", getRoot())
		, m_minError("min_error", getRoot())
		, m_maxError("max_error", getRoot())
		, m_meanError("mean_error", getRoot())
		, m_stdDev("std_dev", getRoot())
	{
		lv_obj_set_size(getRoot(), LV_PCT(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(getRoot(), LV_FLEX_FLOW_ROW_WRAP);
		lv_obj_set_flex_align(getRoot(), LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
		for (size_t i = 0; i < lv_obj_get_child_cnt(getRoot()); i++)
		{
			lv_obj_t* child = lv_obj_get_child(getRoot(), i);
			lv_obj_set_size(child, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		}
	}

	void HeightmapStatistics::setStatistics(
		size_t numPoints, double area, double minError, double maxError, double meanError, double stdDev)
	{
		UI_LOCK();
		m_numPoints.setText(fmt::format(fmt::runtime(_("heightmap_num_points")), numPoints));
		m_area.setText(fmt::format(fmt::runtime(_("heightmap_area")), area));
		m_minError.setText(fmt::format(fmt::runtime(_("heightmap_min_error")), minError));
		m_maxError.setText(fmt::format(fmt::runtime(_("heightmap_max_error")), maxError));
		m_meanError.setText(fmt::format(fmt::runtime(_("heightmap_mean_error")), meanError));
		m_stdDev.setText(fmt::format(fmt::runtime(_("heightmap_std_dev")), stdDev));
	}

	HeightmapView::HeightmapView(LvObj& parent)
		: View("HeightmapView", parent, layout_t(0, 0, 100, 100))
		, m_layoutColDsc{LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}
		, m_layoutRowDsc{LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}
		, m_heightmap("heightmap", getRoot(), layout_t(0, 0, 100, 100))
		, m_heightmapList("heightmap_list", getRoot())
		, m_statistics("heightmap_statistics", getRoot())
		, m_renderMode(getRoot(), *getPresenter().get())
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);
		m_heightmap.addStyle(Themes::getLvglStyles().card);
		m_heightmapList.addStyle(Themes::getLvglStyles().card);
		m_statistics.addStyle(Themes::getLvglStyles().card);
		m_renderMode.addStyle(Themes::getLvglStyles().card);

		lv_obj_set_layout(getRoot(), LV_LAYOUT_GRID);

		setGridDsc(m_layoutColDsc, m_layoutRowDsc);
		setGridCell(m_heightmap, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 2);
		setGridCell(m_heightmapList, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		setGridCell(m_renderMode, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 1, 1);
		setGridCell(m_statistics, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 2, 1);

		m_heightmap.setResolution(200, 200);

		// List
		m_heightmapList.setTitle(_("heightmap_list_header"));
		m_heightmapList.setListGrow(1);

		clear();
	}

	const size_t HeightmapView::getHeightmapCount() const
	{
		UI_LOCK();
		return m_heightmapList.getItemCount();
	}

	void HeightmapView::setHeightmapCount(const size_t count)
	{
		UI_LOCK();
		m_heightmapList.setItemCount(count, *this);
	}

	void HeightmapView::setHeightmapName(const size_t index, const std::string& name)
	{
		UI_LOCK();

		auto item = m_heightmapList.getItem(index);
		if (item == nullptr)
		{
			LOG_WARN("Can't set heightmap {:d} name to {:s}, index invalid", index, name);
			return;
		}

		item->setLabel(name);
	}

	void HeightmapView::setSelectedHeightmap(const int32_t index)
	{
		UI_LOCK();
		for (auto item : m_heightmapList)
		{
			if (item == nullptr)
			{
				LOG_WARN("Heightmap item is null");
				continue;
			}
			item->setSelected(item->getIndex() == index);
		}
	}

	void HeightmapView::setShownHeightmapName(std::string_view name)
	{
		UI_LOCK();
		m_heightmap.setTitle(fmt::format(fmt::runtime(_("heightmap_title")), name));
	}

	void HeightmapView::addMeasurementPoint(float x, float y)
	{
		UI_LOCK();
		size_t px, py;
		if (!m_heightmap.posToPx(x, y, px, py))
		{
			LOG_WARN("Measurement ({:g}, {:g}) not within axis bounds", x, y);
			return;
		}
		uint32_t width, height;
		m_heightmap.getResolution(width, height);
		m_heightmap.getCanvas().drawCirclePx(
			{(int32_t)px, (int32_t)(height - py)}, 1, lv_palette_main(LV_PALETTE_GREY), LV_OPA_20);
	}

	void HeightmapView::clear()
	{
		UI_LOCK();
		m_heightmap.clear();

		setShownHeightmapName("");
		drawGrid();
		setStatistics(0, 0.0, 0.0, 0.0, 0.0, 0.0);
	}

	void HeightmapView::setStatistics(
		size_t numPoints, double area, double minError, double maxError, double meanError, double stdDev)
	{
		UI_LOCK();
		m_statistics.setStatistics(numPoints, area, minError, maxError, meanError, stdDev);
	}

	void HeightmapView::onShow() {}
} // namespace UI

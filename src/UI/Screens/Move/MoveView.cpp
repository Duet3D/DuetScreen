#include "MoveView.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include "utils/StorageHelper.h"

namespace UI
{
	static constexpr float s_distances[] = {0.1f, 0.5f, 1, 5, 10, 25, 50};						   // mm
	static constexpr float s_relMoveValues[] = {-50, -10, -1.0f, -0.1f, 0.1f, 1.0f, 10.0f, 50.0f}; // mm
	static constexpr uint32_t s_feedRates[] = {300, 100, 50, 20, 10, 5};						   // mm/s
	static uint32_t s_currentDistanceIndex = 4;
	static uint32_t s_currentFeedRate = 50;

	MoveView::MoveView(LvObj& parent)
		: View("move_view", parent, layout_t(0, 0, 100, 100))
		, m_layoutColDsc{LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}
		, m_layoutRowDsc{LV_GRID_CONTENT, LV_GRID_FR(3), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}
		, m_topBarCont("topbar", getRoot())
		, m_bottomBarCont("bottombar", getRoot())
		, m_homeAll("home_all", m_topBarCont, _("home_all"))
		, m_trueBedLevel("true_bed_level", m_topBarCont, _("true_bed_level"))
		, m_meshBedLevel("mesh_bed_level", m_topBarCont, _("mesh_bed_level"))
		, m_heightmap("heightmap", m_topBarCont, _("heightmap"))
		, m_disableMotors("disable_motors", m_topBarCont, _("disable_motors"))
		, m_axisControlCont("axis_control", getRoot())
		, m_xyControl("xy_control", m_axisControlCont)
		, m_zControl("z_control", m_axisControlCont)
		, m_genericAxisControls("generic_axis_controls", m_axisControlCont)
		, m_axisList("axis_control_list", m_axisControlCont)
		, m_extruderControl("extruder_control", m_axisControlCont)
		, m_distances("feed_rates", m_bottomBarCont)
		, m_numberpad("numberpad", getRoot(), layout_t(0, 0, 50, 70))
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);

		m_homeAll.addStyle(Themes::getLvglStyles().actionBtn, 0);
		m_trueBedLevel.addStyle(Themes::getLvglStyles().actionBtn, 0);
		m_meshBedLevel.addStyle(Themes::getLvglStyles().actionBtn, 0);
		m_disableMotors.addStyle(Themes::getLvglStyles().actionBtn, 0);

		// Layout
		setGridDsc(m_layoutColDsc, m_layoutRowDsc);
		setGridCell(m_topBarCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
		setGridCell(m_axisControlCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		setGridCell(m_bottomBarCont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 2, 1);

		// Top Bar
		constexpr lv_coord_t pad = 10;
		m_topBarCont.setHeight(LV_SIZE_CONTENT);
		m_topBarCont.setStylePad(pad, LV_PART_MAIN, Padding::ALL);
		m_topBarCont.setStylePad(pad, LV_PART_MAIN, Padding::COLUMN);
		m_topBarCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_topBarCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_homeAll.setHeight(LV_SIZE_CONTENT);
		m_trueBedLevel.setHeight(LV_SIZE_CONTENT);
		m_meshBedLevel.setHeight(LV_SIZE_CONTENT);
		m_heightmap.setHeight(LV_SIZE_CONTENT);
		m_disableMotors.setHeight(LV_SIZE_CONTENT);
		m_homeAll.setFlexGrow(1);
		m_trueBedLevel.setFlexGrow(1);
		m_meshBedLevel.setFlexGrow(1);
		m_heightmap.setFlexGrow(1);
		m_disableMotors.setFlexGrow(1);

		// Axis Control
		m_axisControlCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_xyControl.setSize(LV_PCT(30), LV_PCT(100));
		m_xyControl.setJogCallback(
			[this](char axis_letter, bool forward)
			{
				m_presenter->moveAxisRelative(
					axis_letter, (forward ? 1 : -1) * s_distances[s_currentDistanceIndex], s_currentFeedRate);
			});
		m_xyControl.setHomeXYCallback(
			[this]()
			{
				m_presenter->homeAxis('X');
				m_presenter->homeAxis('Y');
			});
		m_xyControl.setHomeXCallback([this]() { m_presenter->homeAxis('X'); });
		m_xyControl.setHomeYCallback([this]() { m_presenter->homeAxis('Y'); });

		m_xyControl.setXLabelCallback([this](float position) { configureNumberpadForAxis('X', position); });
		m_xyControl.setYLabelCallback([this](float position) { configureNumberpadForAxis('Y', position); });

		m_zControl.setSize(LV_PCT(10), LV_PCT(100));
		m_zControl.setAxisLetter('Z');
		m_zControl.setJogCallback(
			[this](char axis_letter, bool forward)
			{
				m_presenter->moveAxisRelative(
					axis_letter, (forward ? 1 : -1) * s_distances[s_currentDistanceIndex], s_currentFeedRate);
			});
		m_zControl.setHomeCallback([this](char axis_letter) { m_presenter->homeAxis(axis_letter); });
		m_zControl.setLabelCallback([this](char axis_letter, float position)
									{ configureNumberpadForAxis(axis_letter, position); });

		m_genericAxisControls.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_genericAxisControls.setListSize(LV_SIZE_CONTENT, LV_PCT(100));
		// m_genericAxisControls.setFlexGrow(1);
		m_genericAxisControls.setMaxWidth(LV_PCT(20));
		m_genericAxisControls.setListFlow(LV_FLEX_FLOW_ROW);
		m_genericAxisControls.addStyle(Themes::getLvglStyles().no_border);
		m_genericAxisControls.addStyle(Themes::getLvglStyles().pad_zero);
		m_genericAxisControls.getListContainer().addStyle(Themes::getLvglStyles().pad_zero);

		m_axisList.setFlexGrow(1);
		m_axisList.setHeight(LV_PCT(100));
		m_axisList.setVisible(false);

		m_extruderControl.setHeight(LV_PCT(100));
		m_extruderControl.setFlexGrow(1);
		m_extruderControl.setToolCallback([this](size_t index) { m_presenter->toggleToolState(index); });
		m_extruderControl.setFilamentCallback(
			[this](const std::string& filament)
			{
				if (filament.empty())
				{
					m_presenter->unloadFilament();
				}
				else
				{
					m_presenter->loadFilament(filament);
				}
			});
		m_extruderControl.setDistanceCallback(
			[this](size_t index, float distance)
			{
				openModal(&m_numberpad);
				m_numberpad.setHeader(_("extrude_distance_header"));
				m_numberpad.setValue(distance);
				m_numberpad.setMinValue(0);
				m_numberpad.setMaxValue(1000);
				m_numberpad.setConfirmCallback([this, index](float distance)
											   { m_extruderControl.setDistanceValue(index, distance); });
			});
		m_extruderControl.setFeedrateCallback(
			[this](size_t index, float feedrate)
			{
				openModal(&m_numberpad);
				m_numberpad.setHeader(_("extrude_feedrate_header"));
				m_numberpad.setValue(feedrate);
				m_numberpad.setMinValue(0);
				m_numberpad.setMaxValue(100); // mm/s
				m_numberpad.setConfirmCallback([this, index](float feedrate)
											   { m_extruderControl.setFeedrateValue(index, feedrate); });
			});
		m_extruderControl.setExtrudeCallback([this](float distance, float feedrate)
											 { m_presenter->extrude(distance, feedrate); });

		m_homeAll.addClickedCallback(onHomeAllEvent, this);
		m_trueBedLevel.addClickedCallback(onTrueBedLevelEvent, this);
		m_meshBedLevel.addClickedCallback(onMeshBedLevelEvent, this);
		m_heightmap.addClickedCallback(onHeightmapEvent, this);
		m_disableMotors.addClickedCallback(onDisableMotorsEvent, this);

		// Bottom Bar
		m_bottomBarCont.setHeight(LV_SIZE_CONTENT);
		m_bottomBarCont.setStylePad(0, LV_PART_MAIN, Padding::ALL);
		m_bottomBarCont.setStylePad(0, LV_PART_MAIN, Padding::COLUMN);
		m_bottomBarCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_bottomBarCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		assert(s_currentDistanceIndex < ARRAY_SIZE(s_distances));

		m_distances.addStyle(Themes::getLvglStyles().no_border);
		m_distances.setTitle(_("move_distance"));
		m_distances.setHeight(LV_SIZE_CONTENT);
		m_distances.setFlexGrow(1);
		m_distances.setListSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_distances.setListFlow(LV_FLEX_FLOW_ROW);
		m_distances.setListPad(0);
		m_distances.setItemCount(ARRAY_SIZE(s_distances),
								 [this](size_t i, LvObj& parent)
								 {
									 auto btn = std::make_shared<Button>(
										 fmt::format("{}", i), parent, fmt::format("{}", s_distances[i]));
									 btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
									 btn->addClickedCallback(onDistanceEvent, this);
									 btn->setCheckable(true);
									 btn->setFlexGrow(1);
									 btn->setHeight(LV_SIZE_CONTENT);
									 return btn;
								 });
		m_distances.getItem(s_currentDistanceIndex)->setChecked(true);

		m_numberpad.setHeader("Numberpad Header");
		m_numberpad.hide();
	}

	void MoveView::onHomeAllEvent(lv_event_t* e)
	{
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->homeAll();
	}

	void MoveView::onTrueBedLevelEvent(lv_event_t* e)
	{
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->trueBedLevel();
	}

	void MoveView::onMeshBedLevelEvent(lv_event_t* e)
	{
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->meshBedLevel();
	}

	void MoveView::onHeightmapEvent(lv_event_t* e)
	{
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->heightmap();
	}

	void MoveView::onDisableMotorsEvent(lv_event_t* e)
	{
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->disableMotors();
	}

	void MoveView::onDistanceEvent(lv_event_t* e)
	{
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		lv_obj_t* btn = (lv_obj_t*)lv_event_get_target_obj(e);
		view->m_distances.getItem(s_currentDistanceIndex)->setChecked(false);
		s_currentDistanceIndex = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn));
		view->m_distances.getItem(s_currentDistanceIndex)->setChecked(true);
	}

	void MoveView::onShow()
	{
		m_extruderControl.show();
	}

	void MoveView::onHide() {}

	void MoveView::clear()
	{
		m_xyControl.setXDisabled(true);
		m_xyControl.setYDisabled(true);
		m_xyControl.setXHomed(false);
		m_xyControl.setYHomed(false);
		m_zControl.setDisabled(true);
		m_zControl.setAxisHomed(false);
		m_extruderControl.clear();
	}

	void MoveView::setAxisData(const std::vector<MovePresenter::AxisData>& axis_data)
	{
		UI_LOCK();
		bool has_x = false;
		bool has_y = false;
		bool has_z = false;
		std::vector<MovePresenter::AxisData> axis_data_excluding_xyz;
		axis_data_excluding_xyz.reserve(axis_data.size());
		for (const auto& axis : axis_data)
		{
			if (axis.letter == 'X')
			{
				has_x = true;
				m_xyControl.setXPosition(axis.position);
				m_xyControl.setXHomed(axis.homed);
			}
			else if (axis.letter == 'Y')
			{
				has_y = true;
				m_xyControl.setYPosition(axis.position);
				m_xyControl.setYHomed(axis.homed);
			}
			else if (axis.letter == 'Z')
			{
				has_z = true;
				m_zControl.setAxisPosition(axis.position);
				m_zControl.setAxisHomed(axis.homed);
			}
			else
			{
				axis_data_excluding_xyz.push_back(axis);
			}
		}

		setAxisDisabled('X', !has_x);
		setAxisDisabled('Y', !has_y);
		setAxisDisabled('Z', !has_z);

#if DEBUG
		size_t remaining_axis_count = axis_data.size() - (has_x ? 1 : 0) - (has_y ? 1 : 0) - (has_z ? 1 : 0);
		if (remaining_axis_count != axis_data_excluding_xyz.size())
		{
			LOG_ERROR("Remaining axis count does not match the number of provided axis letters. "
					  "Expected: {}, Actual: {}",
					  remaining_axis_count,
					  axis_data_excluding_xyz.size());
			return;
		}
#endif

		LOG_DBG("Remaining axis count: {}", axis_data_excluding_xyz.size());
		m_genericAxisControls.setVisible(!axis_data_excluding_xyz.empty());

		m_genericAxisControls.setItemCount(
			axis_data_excluding_xyz.size(),
			[this](size_t i, LvObj& parent)
			{
				auto control = std::make_shared<GenericAxisControl>(fmt::format("{}", i), parent);
				lv_coord_t width = m_zControl.getWidth();
				control->setSize(width, LV_PCT(100));
				control->setJogCallback(
					[this](char axis_letter, bool forward)
					{
						m_presenter->moveAxisRelative(
							axis_letter, (forward ? 1 : -1) * s_distances[s_currentDistanceIndex], s_currentFeedRate);
					});
				control->setHomeCallback([this](char axis_letter) { m_presenter->homeAxis(axis_letter); });
				control->setLabelCallback([this](char axis_letter, float position)
										  { configureNumberpadForAxis(axis_letter, position); });
				return control;
			});

		for (size_t i = 0; i < axis_data_excluding_xyz.size(); ++i)
		{
			const MovePresenter::AxisData& data = axis_data_excluding_xyz[i];
			auto control = m_genericAxisControls.getItem(i);
			control->setAxisLetter(data.letter);
			control->setAxisPosition(data.position);
			control->setAxisHomed(data.homed);
		}

		m_axisDataListPtr = &axis_data;
	}

	void MoveView::setPositionType(const MovePresenter::PositionType type) {}

	void MoveView::setAxisPosition(char axis_letter, float position)
	{
		UI_LOCK();
		if (axis_letter == 'X')
		{
			m_xyControl.setXPosition(position);
		}
		else if (axis_letter == 'Y')
		{
			m_xyControl.setYPosition(position);
		}
		else if (axis_letter == 'Z')
		{
			m_zControl.setAxisPosition(position);
		}
		else
		{
			for (auto& control : m_genericAxisControls.getItems())
			{
				if (control->getAxisLetter() == axis_letter)
				{
					control->setAxisPosition(position);
					return;
				}
			}
			LOG_WARN("No control found for axis letter: {}", axis_letter);
		}
	}

	void MoveView::setAxisHomed(char axis_letter, bool homed)
	{
		UI_LOCK();
		if (axis_letter == 'X')
		{
			m_xyControl.setXHomed(homed);
		}
		else if (axis_letter == 'Y')
		{
			m_xyControl.setYHomed(homed);
		}
		else if (axis_letter == 'Z')
		{
			m_zControl.setAxisHomed(homed);
		}
		else
		{
			for (auto& control : m_genericAxisControls.getItems())
			{
				if (control->getAxisLetter() == axis_letter)
				{
					control->setAxisHomed(homed);
					return;
				}
			}
			LOG_WARN("No control found for axis letter: {}", axis_letter);
		}
	}

	void MoveView::setAxisDisabled(char axis_letter, bool disabled)
	{
		UI_LOCK();
		setAxisJogDisabled(axis_letter, disabled);
		setAxisHomeDisabled(axis_letter, disabled);
	}

	void MoveView::setAxisJogDisabled(char axis_letter, bool disabled)
	{
		UI_LOCK();
		if (axis_letter == 'X')
		{
			m_xyControl.setXJogDisabled(disabled);
		}
		else if (axis_letter == 'Y')
		{
			m_xyControl.setYJogDisabled(disabled);
		}
		else if (axis_letter == 'Z')
		{
			m_zControl.setJogDisabled(disabled);
		}
		else
		{
			for (auto& control : m_genericAxisControls.getItems())
			{
				if (control->getAxisLetter() == axis_letter)
				{
					control->setJogDisabled(disabled);
					return;
				}
			}
		}
	}

	void MoveView::setAxisHomeDisabled(char axis_letter, bool disabled)
	{
		UI_LOCK();
		if (axis_letter == 'X')
		{
			m_xyControl.setXHomeDisabled(disabled);
		}
		else if (axis_letter == 'Y')
		{
			m_xyControl.setYHomeDisabled(disabled);
		}
		else if (axis_letter == 'Z')
		{
			m_zControl.setHomeDisabled(disabled);
		}
		else
		{
			for (auto& control : m_genericAxisControls.getItems())
			{
				if (control->getAxisLetter() == axis_letter)
				{
					control->setHomeDisabled(disabled);
					return;
				}
			}
		}
	}

	void MoveView::setToolCount(const size_t count)
	{
		m_extruderControl.setToolCount(count);
	}

	void MoveView::setToolName(const size_t index, const std::string& name)
	{
		m_extruderControl.setToolName(index, name);
	}

	void MoveView::setCurrentTool(const int32_t index)
	{
		m_extruderControl.setCurrentTool(index);
	}

	void MoveView::setAxisCount(const size_t count)
	{
		List<AxisItem>& list = m_axisList.getAxisItems();
		list.setItemCount(
			count,
			[this](size_t i, LvObj& parent)
			{
				auto item = std::make_shared<AxisItem>(i, parent);
				item->setJogAmounts(s_relMoveValues, ARRAY_SIZE(s_relMoveValues));
				item->setJogCallback(
					[this](size_t axis_index, size_t jog_index, void* user_data)
					{ m_presenter->moveAxisRelative(axis_index, s_relMoveValues[jog_index], s_currentFeedRate); },
					this);
				item->setHomeCallback([this](size_t axis_index, void* user_data) { m_presenter->homeAxis(axis_index); },
									  this);
				return item;
			});
	}

	std::shared_ptr<AxisItem> MoveView::getAxisItem(size_t index)
	{
		return m_axisList.getAxisItems().getItem(index);
	}

	void MoveView::configureNumberpadForAxis(char axis_letter, float position)
	{
		openModal(&m_numberpad);
		m_numberpad.setHeader(utils::format(_("move_set_position"), axis_letter));
		m_numberpad.setValue(position);
		if (m_axisDataListPtr)
		{
			for (auto& axis : *m_axisDataListPtr)
			{
				if (axis.letter == axis_letter)
				{
					m_numberpad.setMinValue(axis.min);
					m_numberpad.setMaxValue(axis.max);
					break;
				}
			}
		}
		m_numberpad.setConfirmCallback([this, axis_letter](float value)
									   { m_presenter->moveAxisAbsolute(axis_letter, value, s_currentFeedRate); });
	}
} // namespace UI

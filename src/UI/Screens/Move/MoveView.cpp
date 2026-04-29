#include "MoveView.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"

namespace UI
{
	static std::vector<float> s_distances;	  // mm
	static std::vector<uint32_t> s_feedRates; // mm/s
	static size_t s_currentDistanceIndex = 4;
	static size_t s_currentFeedrateIndex = 3;

	static float getSelectedDistance()
	{
		ZoneScoped;
		s_currentDistanceIndex = std::min(s_currentDistanceIndex, s_distances.empty() ? 0 : s_distances.size() - 1);
		return s_distances[s_currentDistanceIndex];
	}

	static uint32_t getSelectedFeedrate()
	{
		ZoneScoped;
		s_currentFeedrateIndex = std::min(s_currentFeedrateIndex, s_feedRates.empty() ? 0 : s_feedRates.size() - 1);
		return s_feedRates[s_currentFeedrateIndex];
	}

	MoveView::MoveView(const std::string& name, LvObj& parent)
		: View(name, parent, layout_t(0, 0, 100, 100))
	{
		ZoneScoped;
		UI_LOCK();

		addStyle(Themes::getLvglStyles().bg_dark);
		m_babyStepCont.addStyle(Themes::getLvglStyles().card);

		/* Layout */
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		auto distances = StorageHelper::getData(ID_MOVE_DISTANCES);
		if (!distances.empty())
		{
			s_distances = std::move(distances);
		}
		auto feedRates = StorageHelper::getData(ID_MOVE_FEEDRATES);
		if (!feedRates.empty())
		{
			s_feedRates = std::move(feedRates);
		}

		m_centralRow.setWidth(LV_PCT(100));
		m_centralRow.setFlexGrow(1);
		m_centralRow.setStylePad(0);

		/* Message Box */
		m_messageBox.setOkBtnText(_("common.yes"));
		m_messageBox.setCancelBtnText(_("common.no"));
		m_messageBox.okVisible(true);
		m_messageBox.cancelVisible(true);
		m_messageBox.getOkBtn().addStyle(Themes::getLvglStyles().actionBtn);

		/* Axis Control */
		m_axisControlCont.setFlag(LV_OBJ_FLAG_SCROLLABLE, false);
		m_axisControlCont.setHeight(LV_PCT(100));
		m_axisControlCont.setFlexGrow(1);
		m_axisControlCont.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_axisControlCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_xyControl.setSize(LV_PCT(38), LV_PCT(100));
		m_xyControl.setDisableMotorsCallback(
			[this]()
			{
				if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				{
					m_presenter->disableMotors();
					return;
				}
				m_messageBox.setTitle(_("move.disable_motors_confirm.title"));
				m_messageBox.setText(_("move.disable_motors_confirm.text"));
				m_messageBox.setOkCallback([this]() { m_presenter->disableMotors(); });
				openModal(&m_messageBox);
			});
		m_xyControl.setJogCallback(
			[this](char axis_letter, bool forward)
			{
				m_presenter->moveAxisRelative(
					axis_letter, (forward ? 1 : -1) * getSelectedDistance(), getSelectedFeedrate());
			});
		m_xyControl.setHomeAllCallback(
			[this]()
			{
				if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				{
					m_presenter->homeAll();
					return;
				}
				m_messageBox.setTitle(_("move.home_all_confirm.title"));
				m_messageBox.setText(_("move.home_all_confirm.text"));
				m_messageBox.setOkCallback([this]() { m_presenter->homeAll(); });
				openModal(&m_messageBox);
			});
		m_xyControl.setHomeXYCallback(
			[this]()
			{
				if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				{
					m_presenter->homeAxis('X');
					m_presenter->homeAxis('Y');
					return;
				}
				m_messageBox.setTitle(_("move.home_xy_confirm.title"));
				m_messageBox.setText(_("move.home_xy_confirm.text"));
				m_messageBox.setOkCallback(
					[this]()
					{
						m_presenter->homeAxis('X');
						m_presenter->homeAxis('Y');
					});
				openModal(&m_messageBox);
			});
		m_xyControl.setHomeXCallback(
			[this]()
			{
				if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				{
					m_presenter->homeAxis('X');
					return;
				}
				m_messageBox.setTitle(_("move.home_generic_confirm.title", 'X'));
				m_messageBox.setText(_("move.home_generic_confirm.text", 'X'));
				m_messageBox.setOkCallback([this]() { m_presenter->homeAxis('X'); });
				openModal(&m_messageBox);
			});
		m_xyControl.setHomeYCallback(
			[this]()
			{
				if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				{
					m_presenter->homeAxis('Y');
					return;
				}
				m_messageBox.setTitle(_("move.home_generic_confirm.title", 'Y'));
				m_messageBox.setText(_("move.home_generic_confirm.text", 'Y'));
				m_messageBox.setOkCallback([this]() { m_presenter->homeAxis('Y'); });
				openModal(&m_messageBox);
			});

		m_xyControl.setXLabelCallback([this](float position) { configureNumberpadForAxis('X', position); });
		m_xyControl.setYLabelCallback([this](float position) { configureNumberpadForAxis('Y', position); });

		m_zControl.setSize(LV_PCT(13), LV_PCT(100));
		m_zControl.setMinWidth(LV_SIZE_CONTENT);
		m_zControl.setAxisLetter('Z');
		m_zControl.setJogCallback(
			[this](char axis_letter, bool forward)
			{
				m_presenter->moveAxisRelative(
					axis_letter, (forward ? 1 : -1) * getSelectedDistance(), getSelectedFeedrate());
			});
		m_zControl.setHomeCallback(
			[this](char axis_letter)
			{
				if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
				{
					m_presenter->homeAxis(axis_letter);
					return;
				}
				m_messageBox.setTitle(_("move.home_generic_confirm.title", axis_letter));
				m_messageBox.setText(_("move.home_generic_confirm.text", axis_letter));
				m_messageBox.setOkCallback([this, axis_letter]() { m_presenter->homeAxis(axis_letter); });
				openModal(&m_messageBox);
			});
		m_zControl.setLabelCallback([this](char axis_letter, float position)
									{ configureNumberpadForAxis(axis_letter, position); });

		m_genericAxisControls.setFlexGrow(1);
		m_genericAxisControls.setSize(LV_SIZE_CONTENT, LV_PCT(100));
		m_genericAxisControls.setListSize(LV_SIZE_CONTENT, LV_PCT(100));
		// m_genericAxisControls.setFlexGrow(1);
		m_genericAxisControls.setMaxWidth(LV_SIZE_CONTENT);
		m_genericAxisControls.setListFlow(LV_FLEX_FLOW_ROW);
		m_genericAxisControls.addStyle(Themes::getLvglStyles().no_border);
		m_genericAxisControls.addStyle(Themes::getLvglStyles().pad_zero);
		m_genericAxisControls.getListContainer().addStyle(Themes::getLvglStyles().pad_zero);

		/* Babystepping */
		m_babyStepCont.setSize(LV_PCT(22), LV_PCT(100));
		m_babyStepCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_babyStepCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_babystep.setSize(LV_PCT(100), LV_PCT(100));
		m_babystep.setFlexGrow(1);
		m_babystep.setNumberPad(&m_numberpad);

		/* Bottom Bar */
		m_bottomBarCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_bottomBarCont.setStylePad(0, LV_PART_MAIN, Padding::ALL);
		m_bottomBarCont.setStylePad(0, LV_PART_MAIN, Padding::COLUMN);
		m_bottomBarCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_bottomBarCont.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		/* Distances */
		if (s_currentDistanceIndex >= std::size(s_distances))
			s_currentDistanceIndex = std::size(s_distances) - 1;
		assert(s_currentDistanceIndex < std::size(s_distances));

		m_distances.addStyle(Themes::getLvglStyles().no_border);
		m_distances.setTitle(_("move.distance", Units::getDisplayedDistanceUnit()));
		m_distances.getHeader().setWidth(LV_SIZE_CONTENT);
		m_distances.getHeader().setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_distances.getHeader().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_distances.getListContainer().setFlexGrow(1);
		m_distances.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_distances.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_distances.setListSize(LV_PCT(100), LV_PCT(100));
		m_distances.setListFlow(LV_FLEX_FLOW_ROW);
		m_distances.getListContainer().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_distances.setListPad(0);
		m_distances.setItemCount(std::size(s_distances),
								 [this](size_t i, LvObj& parent)
								 {
									 auto btn = std::make_unique<Button>(fmt::format("{}", i), parent);
									 btn->addStyle(Themes::getLvglStyles().long_press);
									 btn->setText(fmt::format("{}", s_distances[i]));
									 btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
									 btn->addClickedCallback(
										 [this, i](lv_event_t*)
										 {
											 if (auto item = m_distances.getItem(s_currentDistanceIndex))
											 {
												 item->setChecked(false);
											 }
											 s_currentDistanceIndex = i;
											 if (auto item = m_distances.getItem(s_currentDistanceIndex))
											 {
												 item->setChecked(true);
											 }
										 });
									 btn->setCheckable(true);
									 btn->setChecked(i == s_currentDistanceIndex);
									 btn->setFlexGrow(1);
									 btn->setHeight(LV_PCT(100));
									 btn->setMinHeight(LV_SIZE_CONTENT);
									 btn->addEventCallback(
										 [this, i, btn_ptr = btn.get()](lv_event_t* /* e */)
										 {
											 assert(i < std::size(s_distances));

											 float value = s_distances[i];
											 openModal(&m_numberpad);
											 m_numberpad.setHeader(
												 _("move.distance_adjust_header", Units::getDisplayedDistanceUnit()));
											 m_numberpad.setValue(value);
											 m_numberpad.setMinValue(0.01f);
											 m_numberpad.setMaxValue(1000.0f);
											 m_numberpad.setConfirmCallback(
												 [btn_ptr, i](float value)
												 {
													 s_distances[i] = value;
													 btn_ptr->setText(fmt::format("{}", s_distances[i]));
													 StorageHelper::setData(ID_MOVE_DISTANCES, s_distances);
												 });
										 },
										 LV_EVENT_LONG_PRESSED);
									 return btn;
								 });

		m_distances.updateLayout();

		/* Feedrates */

		if (s_currentFeedrateIndex >= std::size(s_feedRates))
			s_currentFeedrateIndex = std::size(s_feedRates) - 1;
		assert(s_currentFeedrateIndex < std::size(s_feedRates));

		m_feedrates.addStyle(Themes::getLvglStyles().no_border);
		m_feedrates.setTitle(_("move.feedrate", Units::getDisplayedSpeedUnit()));
		m_distances.getHeader().addEventCallback(
			[this](lv_event_t*) { m_feedrates.getHeader().setWidth(m_distances.getHeader().getWidth()); },
			LV_EVENT_SIZE_CHANGED);
		m_distances.getHeader().sendEvent(LV_EVENT_SIZE_CHANGED);
		m_feedrates.getHeader().setStyleTextAlign(LV_TEXT_ALIGN_CENTER);
		m_feedrates.getHeader().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_feedrates.getListContainer().setFlexGrow(1);
		m_feedrates.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_feedrates.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_feedrates.setListSize(LV_PCT(100), LV_PCT(100));
		m_feedrates.setListFlow(LV_FLEX_FLOW_ROW);
		m_feedrates.getListContainer().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_feedrates.setListPad(0);
		m_feedrates.setItemCount(std::size(s_feedRates),
								 [this](size_t i, LvObj& parent)
								 {
									 auto btn = std::make_unique<Button>(fmt::format("{}", i), parent);
									 btn->addStyle(Themes::getLvglStyles().long_press);
									 btn->setText(fmt::format("{}", s_feedRates[i]));
									 btn->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
									 btn->addClickedCallback(
										 [this, i](lv_event_t*)
										 {
											 if (auto item = m_feedrates.getItem(s_currentFeedrateIndex))
											 {
												 item->setChecked(false);
											 }
											 s_currentFeedrateIndex = i;
											 if (auto item = m_feedrates.getItem(s_currentFeedrateIndex))
											 {
												 item->setChecked(true);
											 }
										 });
									 btn->setCheckable(true);
									 btn->setChecked(i == s_currentFeedrateIndex);
									 btn->setFlexGrow(1);
									 btn->setHeight(LV_PCT(100));
									 btn->setMinHeight(LV_SIZE_CONTENT);
									 btn->addEventCallback(
										 [this, i, btn_ptr = btn.get()](lv_event_t* /* e */)
										 {
											 assert(i < std::size(s_feedRates));

											 float value = static_cast<float>(s_feedRates[i]);
											 openModal(&m_numberpad);
											 m_numberpad.setHeader(
												 _("move.feedrate_adjust_header", Units::getDisplayedSpeedUnit()));
											 m_numberpad.setValue(value);
											 m_numberpad.setMinValue(0.01f);
											 m_numberpad.setMaxValue(1000.0f);
											 m_numberpad.setConfirmCallback(
												 [btn_ptr, i](float value)
												 {
													 s_feedRates[i] = static_cast<uint32_t>(std::lround(value));
													 btn_ptr->setText(fmt::format("{}", s_feedRates[i]));
													 StorageHelper::setData(ID_MOVE_FEEDRATES, s_feedRates);
												 });
										 },
										 LV_EVENT_LONG_PRESSED);
									 return btn;
								 });
	}

	void MoveView::onHomeAllEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->homeAll();
	}

	void MoveView::onDisableMotorsEvent(lv_event_t* e)
	{
		ZoneScoped;
		UI_LOCK();
		MoveView* view = static_cast<MoveView*>(lv_event_get_user_data(e));
		view->m_presenter->disableMotors();
	}

	void MoveView::setDisabled(bool disabled)
	{
		ZoneScoped;
		m_axisControlCont.setState(LV_STATE_DISABLED, disabled, true);

		// m_xyControl.setHomeAllDisabled(true);
		// m_xyControl.setDisableMotorsDisabled(true);
		// m_xyControl.setXDisabled(true);
		// m_xyControl.setYDisabled(true);
		// m_xyControl.setXHomed(false);
		// m_xyControl.setYHomed(false);
		// m_zControl.setDisabled(true);
		// m_zControl.setAxisHomed(false);
	}

	void MoveView::clear()
	{
		ZoneScoped;
		m_genericAxisControls.clear();
	}

	void MoveView::setAxisData(const std::vector<MovePresenter::AxisData>& axis_data)
	{
		ZoneScoped;
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
				m_xyControl.setXPosition(Units::convertDistanceToCurrentDisplayedUnit(axis.position));
				m_xyControl.setXHomed(axis.homed);
				m_xyControl.setXHomeDisabled(axis.home_disabled);
				m_xyControl.setXJogDisabled(axis.jog_disabled);
			}
			else if (axis.letter == 'Y')
			{
				has_y = true;
				m_xyControl.setYPosition(Units::convertDistanceToCurrentDisplayedUnit(axis.position));
				m_xyControl.setYHomed(axis.homed);
				m_xyControl.setYHomeDisabled(axis.home_disabled);
				m_xyControl.setYJogDisabled(axis.jog_disabled);
			}
			else if (axis.letter == 'Z')
			{
				has_z = true;
				m_zControl.setAxisPosition(Units::convertDistanceToCurrentDisplayedUnit(axis.position));
				m_zControl.setAxisHomed(axis.homed);
				m_zControl.setHomeDisabled(axis.home_disabled);
				m_zControl.setJogDisabled(axis.jog_disabled);
			}
			else
			{
				axis_data_excluding_xyz.push_back(axis);
			}
		}

		if (!has_x)
			setAxisDisabled('X', true);

		if (!has_y)
			setAxisDisabled('Y', true);

		if (!has_z)
			setAxisDisabled('Z', true);

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
				auto control = std::make_unique<GenericAxisControl>(fmt::format("{}", i), parent);
				lv_coord_t width = m_zControl.getWidth();
				control->setSize(width, LV_PCT(100));
				control->setMinWidth(LV_SIZE_CONTENT);
				control->setJogCallback(
					[this](char axis_letter, bool forward)
					{
						m_presenter->moveAxisRelative(
							axis_letter, (forward ? 1 : -1) * getSelectedDistance(), getSelectedFeedrate());
					});
				control->setHomeCallback(
					[this](char axis_letter)
					{
						if (!StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS))
						{
							m_presenter->homeAxis(axis_letter);
							return;
						}
						m_messageBox.setTitle(_("move.home_generic_confirm.title", axis_letter));
						m_messageBox.setText(_("move.home_generic_confirm.text", axis_letter));
						m_messageBox.setOkCallback([this, axis_letter]() { m_presenter->homeAxis(axis_letter); });
						openModal(&m_messageBox);
					});
				control->setLabelCallback([this](char axis_letter, float position)
										  { configureNumberpadForAxis(axis_letter, position); });
				return control;
			});

		for (size_t i = 0; i < axis_data_excluding_xyz.size(); ++i)
		{
			const MovePresenter::AxisData& data = axis_data_excluding_xyz[i];
			if (auto item = m_genericAxisControls.getItem(i))
			{
				item->setAxisLetter(data.letter);
				item->setAxisPosition(Units::convertDistanceToCurrentDisplayedUnit(data.position));
				item->setAxisHomed(data.homed);
				item->setHomeDisabled(data.home_disabled);
				item->setJogDisabled(data.jog_disabled);
			}
			else
			{
				LOG_WARN("No control found for axis letter: {}", data.letter);
			}
		}

		m_axisDataListPtr = &axis_data;
	}

	void MoveView::setAxisPosition(char axis_letter, float position)
	{
		ZoneScoped;
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
		ZoneScoped;
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
		ZoneScoped;
		UI_LOCK();
		setAxisJogDisabled(axis_letter, disabled);
		setAxisHomeDisabled(axis_letter, disabled);
	}

	void MoveView::setAxisJogDisabled(char axis_letter, bool disabled)
	{
		ZoneScoped;
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
		ZoneScoped;
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

	void MoveView::setHomeAllDisabled(bool disabled)
	{
		ZoneScoped;
		m_xyControl.setHomeAllDisabled(disabled);
	}

	void MoveView::setDisableMotorsDisabled(bool disabled)
	{
		ZoneScoped;
		m_xyControl.setDisableMotorsDisabled(disabled);
	}

	void MoveView::onShow()
	{
		ZoneScoped;
		closeAllModals();
	}

	void MoveView::configureNumberpadForAxis(char axis_letter, float position)
	{
		ZoneScoped;
		openModal(&m_numberpad);
		m_numberpad.setHeader(_("move.set_position", axis_letter));
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
									   { m_presenter->moveAxisAbsolute(axis_letter, value, getSelectedFeedrate()); });
	}
} // namespace UI

/*
 * MotionSystemPanel.cpp
 *
 *  Created on: 2026-03-24
 */

#include "MotionSystemPanel.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Themes/DefaultTheme.h"
#include "i18n/i18n.h"
#include <algorithm>
#include <cmath>

namespace UI
{

	MotionSystemPanel::MotionSystemPanel(const std::string& name, LvObj& parent)
		: LvContainer(name, parent)
	{
		ZoneScoped;
		UI_LOCK();

		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		setGridDsc({LV_GRID_FR(1), LV_GRID_FR(7), LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST},
				   {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST});
		setGridCell(m_headerCont, LV_GRID_ALIGN_STRETCH, 0, 4, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_speedCont, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		setGridCell(m_positionCont, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

		m_headerCont.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_speedCont.setWidth(LV_PCT(70));
		m_positionCont.setWidth(LV_PCT(20));
		m_positionCont.setMinWidth(LV_SIZE_CONTENT);

		m_headerCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_headerCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_speedCont.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_speedCont.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		m_headerCont.addStyle(Themes::getLvglStyles().bg_light);
		m_speedCont.addStyle(Themes::getLvglStyles().bg_light);
		m_headerCont.addStyle(Themes::getLvglStyles().shadow_raised);
		m_speedCont.addStyle(Themes::getLvglStyles().shadow_raised);
		m_title.addStyle(Themes::getLvglStyles().text_emphasis);

		m_tools.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_tools.setListFlow(LV_FLEX_FLOW_ROW);
		m_tools.getListContainer().setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		m_tools.addStyle(Themes::getLvglStyles().bg);
		m_tools.addStyle(Themes::getLvglStyles().shadow_lowered);

		m_tool.hide();

		m_speedFactorLabel.setText("Speed Multiplier:");
		m_speedFactorArc.setWidth(LV_PCT(100));
		m_speedFactorArc.setFlexGrow(1);
		m_speedFactorArc.setRange(0, SpeedFactorArcMax);
		{
			constexpr int32_t arcGap = 60;
			constexpr int32_t arcEnd = 360 - arcGap;
			constexpr int32_t arcRotation = 90 + arcGap / 2;

			m_speedFactorArc.setBgAngles(0, arcEnd);
			m_speedFactorArc.setAngles(0, arcEnd);
			m_speedFactorArc.setRotation(arcRotation);

			m_scale.setAngleRange(arcEnd);
			m_scale.setRotation(arcRotation);
		}
		m_speedFactorArc.setStyleArcWidth(20);
		m_speedFactorArc.setStyleArcWidth(20, LV_PART_INDICATOR);
		m_speedFactorArc.setCenterAlign(LV_ALIGN_CENTER);
		m_speedFactorArc.setMode(LV_ARC_MODE_NORMAL);
		m_speedFactorArc.setValue(SpeedFactorArcMax);
		m_speedFactorArc.setFlag(LV_OBJ_FLAG_CLICKABLE, false);
		m_speedFactorArc.setFlag(LV_OBJ_FLAG_CLICK_FOCUSABLE, false);
		m_speedFactorArc.setStyleBgOpa(LV_OPA_0, LV_PART_KNOB);
		m_speedFactorArc.addStyle(Themes::getLvglStyles().bg);
		m_speedFactorArc.addStyle(Themes::getLvglStyles().shadow_lowered);
		{
			lv_obj_t* obj = m_speedFactorArc.getRootPtr();
			lv_obj_set_style_drop_shadow_color(obj, lv_color_black(), LV_PART_INDICATOR);
			lv_obj_set_style_drop_shadow_opa(obj, LV_OPA_30, LV_PART_INDICATOR);
			lv_obj_set_style_drop_shadow_offset_y(obj, 2, LV_PART_INDICATOR);
		}

		m_scale.setAlign(LV_ALIGN_CENTER, 0, m_speedFactorArc.getStyleProp(LV_STYLE_DROP_SHADOW_OFFSET_Y).num / 2);
		m_scale.setCenterAlign(LV_ALIGN_CENTER);
		m_scale.setMode(LV_SCALE_MODE_ROUND_INNER);
		m_speedFactorArc.addEventCallback(
			[this](lv_event_t*)
			{
				const lv_coord_t arcSize = std::min(m_speedFactorArc.getWidth(), m_speedFactorArc.getHeight());
				const lv_coord_t scaleSize = arcSize - m_speedFactorArc.getStyleProp(LV_STYLE_ARC_WIDTH).num * 2;
				m_scale.setSize(scaleSize, scaleSize);
			},
			LV_EVENT_SIZE_CHANGED);
		m_scale.setSize(LV_PCT(100), LV_PCT(100));

		m_speedFactorValue.setText("100%");
		m_speedFactorValue.setAlign(LV_ALIGN_CENTER, 0, 0);
		m_speedFactorValue.addStyle(Themes::getLvglStyles().text_emphasis);

		m_speedBarLabel.setText("Speed:");
		m_speedBar.addStyle(Themes::getLvglStyles().shadow_raised);
		m_speedBar.setSize(LV_PCT(100), 50);
		m_speedBar.setRange(0, 600);
		m_speedBar.setValues(0, 0, LV_ANIM_OFF);
		m_speedBar.setLabel(_("status.speed_label", 0.0f, 0.0f));
		setSpeedFactor(100);

		m_positions.setListFlow(LV_FLEX_FLOW_COLUMN);
		m_positions.setSize(LV_PCT(100), LV_PCT(100));
		m_positions.getListContainer().setHeight(LV_PCT(100));
		m_positions.setListPad(0);
		m_positions.setStylePad(0);
		m_positions.setItemCount(2);
	}

	void MotionSystemPanel::setTitle(std::string_view title)
	{
		ZoneScoped;
		m_title.setText(title);
	}

	void MotionSystemPanel::setTool(size_t toolIdx)
	{
		ZoneScoped;
		const std::string_view toolName =
			toolIdx >= m_tools.getItemCount() ? "-" : m_tools.getItem(toolIdx)->getLabel().getText();
		m_tool.setText(fmt::format("Current Tool: {:s}", toolName));

		m_tools.iterateListItems([toolIdx](size_t idx, auto& item)
								 { item.getRoot().setState(LV_STATE_CHECKED, idx == toolIdx); });
	}

	void MotionSystemPanel::setToolCount(size_t count)
	{
		ZoneScoped;
		m_tools.setItemCount(count);
	}

	void MotionSystemPanel::setToolInfo(size_t toolIdx, std::string_view name, const void* iconSrc)
	{
		ZoneScoped;
		if (toolIdx >= m_tools.getItemCount())
		{
			return;
		}
		auto toolIcon = m_tools.getItem(toolIdx);
		toolIcon->setName(name);
		toolIcon->setIcon(iconSrc);
	}

	void MotionSystemPanel::setSpeedFactor(uint32_t speedFactorPercent)
	{
		ZoneScoped;
		const auto clamped =
			static_cast<int32_t>(std::min(speedFactorPercent, static_cast<uint32_t>(SpeedFactorArcMax)));
		m_speedFactorArc.setValue(clamped);
		m_speedFactorValue.setText(fmt::format("{:d}%", speedFactorPercent));
	}

	void MotionSystemPanel::setSpeeds(float currentSpeed, float targetSpeed)
	{
		ZoneScoped;
		const int32_t current = sanitizeBarValue(currentSpeed);
		const int32_t target = sanitizeBarValue(targetSpeed);

		m_speedBar.setValues(current, target);
		m_speedBar.setLabel(_("status.speed_label", currentSpeed, targetSpeed));
	}

	void MotionSystemPanel::setPosition(size_t index, char axis_letter, float position)
	{
		ZoneScoped;

		if (index >= m_positions.getItemCount())
		{
			return;
		}

		auto& item = *m_positions.getItem(index);
		item.setAxisLetter(axis_letter);
		item.setPosition(position);
	}

	int32_t MotionSystemPanel::sanitizeBarValue(float value)
	{
		if (!std::isfinite(value))
		{
			return 0;
		}

		return std::max(0, static_cast<int32_t>(std::lround(value)));
	}

	MotionSystemPanel::ToolIcon::ToolIcon(size_t index, LvObj& parent)
		: ListItem(index, parent)
	{
		ZoneScoped;
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		addStyle(Themes::getLvglStyles().bg_light);
		addStyle(Themes::getLvglStyles().shadow_raised);

		m_icon.setSize(LV_PCT(100), 32);
		m_icon.setMinWidth(32);
		m_icon.setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
		m_label.setText("Tool");

		addStyle(Themes::getLvglStyles().outline_primary, LV_STATE_CHECKED);
	}

	MotionSystemPanel::PositionListItem::PositionListItem(size_t index, LvObj& parent)
		: ListItem(index, parent)
	{
		ZoneScoped;
		setFlexFlow(LV_FLEX_FLOW_COLUMN);
		setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		setSize(LV_PCT(100), LV_SIZE_CONTENT);
		setMinWidth(LV_SIZE_CONTENT);
		addStyle(Themes::getLvglStyles().bg_light);
		addStyle(Themes::getLvglStyles().shadow_raised);
		m_axisLabel.addStyle(Themes::getLvglStyles().text_emphasis);
		m_positionLabel.addStyle(Themes::getLvglStyles().text);
	}
} // namespace UI
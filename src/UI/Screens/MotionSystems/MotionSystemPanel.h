/*
 * MotionSystemPanel.h
 *
 *  Created on: 2026-03-24
 */

#pragma once

#include "UI/Components/Bar/CurrentTargetBar.h"
#include "UI/Components/Icon/Icon.h"
#include "UI/Components/LVGL/LvArc.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvLabel.h"
#include "UI/Components/LVGL/LvScale.h"
#include "UI/Components/LVGL/LvSpan.h"
#include "UI/Components/List/List.h"
#include <optional>

namespace UI
{
	class MotionSystemPanel : public LvContainer
	{
	  public:
		class ToolIcon : public ListItem
		{
		  public:
			ToolIcon(size_t index, LvObj& parent);

			void setName(std::string_view name) { m_label.setText(name); }
			void setIcon(std::string_view src) { m_icon.setIcon(src); }

			auto& getIcon() { return m_icon; }
			auto& getLabel() { return m_label; }

		  private:
			Icon m_icon{"icon", getRoot()};
			LvLabel m_label{"label", getRoot()};
		};

		class PositionListItem : public ListItem
		{
		  public:
			PositionListItem(size_t index, LvObj& parent);

			void setAxisLetter(char axis_letter) { m_axisLabel.setText(fmt::format("{}:", axis_letter)); }
			void setPosition(float position) { m_positionLabel.setText(fmt::format("{:.2f}", position)); }

		  private:
			LvLabel m_axisLabel{"axis_label", getRoot()};
			LvLabel m_positionLabel{"position_label", getRoot()};
		};

		MotionSystemPanel(const std::string& name, LvObj& parent);

		void setTitle(std::string_view title);

		void setTool(size_t toolIdx);
		void setToolCount(size_t count);
		void setToolInfo(size_t toolIdx, std::string_view name, std::string_view iconSrc);

		void setSpeedFactor(uint32_t speedFactorPercent);
		void setSpeeds(float currentSpeed, float targetSpeed);

		void setPosition(size_t index, char axis_letter, float position);

		auto& getToolList() { return m_tools; }

	  private:
		static constexpr int32_t SpeedFactorArcMax = 200;

		static int32_t sanitizeBarValue(float value);

		LvContainer m_headerCont{"header_cont", getRoot()};
		LvLabel m_title{"title", m_headerCont};
		List<ToolIcon> m_tools{"tools", m_headerCont};
		LvLabel m_tool{"tool", m_headerCont};

		LvContainer m_speedCont{"speed_cont", getRoot()};
		LvLabel m_speedFactorLabel{"speed_factor_label", m_speedCont};
		LvArc m_speedFactorArc{"speed_factor_arc", m_speedCont};
		LvScale m_scale{"scale", m_speedFactorArc};
		LvLabel m_speedFactorValue{"speed_factor_value", m_speedFactorArc};

		LvLabel m_speedBarLabel{"speed_bar_label", m_speedCont};
		CurrentTargetBar m_speedBar{"speed_bar", m_speedCont};

		LvContainer m_positionCont{"position_cont", getRoot()};
		List<PositionListItem> m_positions{"positions", m_positionCont};
	};
} // namespace UI
/*
 * MotionSystemsView.cpp
 *
 *  Created on: 2026-03-24
 */

#include "MotionSystemsView.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"

namespace UI
{
	MotionSystemsView::MotionSystemsView(const std::string& name, LvObj& parent)
		: View<MotionSystemsPresenter>(name, parent, layout_t(0, 0, 100, 100))
	{
		ZoneScoped;
		UI_LOCK();

		// static constexpr auto colDsc = std::to_array<int32_t>({LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST});
		// static constexpr auto rowDsc = std::to_array<int32_t>({LV_GRID_CONTENT, LV_GRID_FR(1),
		// LV_GRID_TEMPLATE_LAST}); setGridDsc(colDsc, rowDsc);
		setGridDsc({LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST},
				   {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST});

		addStyle(Themes::getLvglStyles().bg_dark);

		setGridCell(m_header, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 0, 1);
		setGridCell(m_systems[0], LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		setGridCell(m_systems[1], LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
		setGridCell(m_hint, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 2, 1);

		m_header.setText("Duet3D Multiple Motion Systems Demo");
		m_header.addStyle(Themes::getLvglStyles().text_header);

		m_systems[0].setTitle(_("motion_system.system_1"));
		m_systems[0].setToolCount(MS_TOOL_COUNT[0]);
		m_systems[0].setToolInfo(0, "Tool 1", "mach_tool1.png");
		m_systems[0].setToolInfo(1, "Tool 2", "mach_tool2.png");
		m_systems[0].setToolInfo(2, "Tool 3", "mach_tool3.png");
		m_systems[0].setTool(-1);

		m_systems[1].setTitle(_("motion_system.system_2"));
		m_systems[1].setToolCount(MS_TOOL_COUNT[1]);
		m_systems[1].setToolInfo(0, "Tool 4", "mach_tool4.png");
		m_systems[1].setToolInfo(1, "Tool 5", "mach_tool5.png");
		m_systems[1].setTool(-1);
		m_systems[1].setSpeedFactor(75);
		m_systems[1].setSpeeds(75.0f, 100.0f);

		m_hint.setText("Turn encoders to adjust the speed factor for each motion system.");
		m_hint.addStyle(Themes::getLvglStyles().text_muted);
	}
} // namespace UI
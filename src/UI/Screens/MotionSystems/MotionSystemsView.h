/*
 * MotionSystemsView.h
 *
 *  Created on: 2026-03-24
 */

#pragma once

#include "MotionSystemPanel.h"
#include "MotionSystemsPresenter.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Core/View.h"
#include <array>
#include <cassert>

namespace UI
{
	class MotionSystemsView : public View<MotionSystemsPresenter>
	{
	  public:
		constexpr static std::array<size_t, 2> MS_TOOL_COUNT{3, 2};

		MotionSystemsView(const std::string& name, LvObj& parent);

		MotionSystemPanel& getMotionSystemPanel(size_t motion_system)
		{
			assert(motion_system < m_systems.size());
			return m_systems[motion_system];
		}
		auto& getMotionSystemPanels() { return m_systems; }

	  private:
		LvLabel m_header{"header", getRoot()};
		std::array<MotionSystemPanel, 2> m_systems{{
			{"system_1", getRoot()},
			{"system_2", getRoot()},
		}};
		LvLabel m_hint{"hint", getRoot()};
	};
} // namespace UI
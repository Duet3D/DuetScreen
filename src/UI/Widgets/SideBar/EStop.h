/*
 * EStop.h
 *
 *  Created on: 2025-08-27
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/DraggableButton.h"
#include "UI/Components/LVGL/LvArcLabel.h"
#include "UI/Core/View.h"
#include "EStopPresenter.h"

namespace UI
{
	class EStop : public View<EStopPresenter, DraggableButton>
	{
	  public:
		EStop(const std::string& name, LvObj& parent);

	  private:
		static void eStopDraggedEvent(float pct, void* user_data);

		LvArcLabel m_label{"label", getRoot()};
	};
} // namespace UI

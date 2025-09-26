/*
 * BabyStep.h
 *
 *  Created on: 2025-09-25
 *      Author: Andy Everitt
 */

#pragma once

#include "BabyStepPresenter.h"
#include "UI/Components/Button/VerticalButtonPanel.h"
#include "UI/Core/View.h"

namespace UI
{
    class BabyStep : public View<BabyStepPresenter>
    {
      public:
		BabyStep(const std::string& name, LvObj& parent);

		void setBabyStepValue(float value);

	  private:
        LvLabel m_header{"header", getRoot()};
		VerticalButtonPanel m_buttonPanel{"button_panel", getRoot()};
    };
} // namespace UI

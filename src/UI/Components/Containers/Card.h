/*
 * Card.h
 *
 *  Created on: 2025-08-07
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvContainer.h"

namespace UI
{
    class Card : public LvContainer
    {
      public:
		Card(const std::string& name, LvObj& parent);
		Card(const std::string& name, LvObj& parent, layout_t layout);

	  private:
        void init();
    };
} // namespace UI

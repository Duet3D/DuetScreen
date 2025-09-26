/*
 * Column.h
 *
 *  Created on: 2025-08-21
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvContainer.h"

namespace UI
{
    class Column : public LvContainer
    {
      public:
		Column(const std::string& name, LvObj& parent);

	  private:
    };
} // namespace UI

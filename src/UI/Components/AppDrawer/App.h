/*
 * App.h
 *
 *  Created on: 2025-08-05
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/List/List.h"

namespace UI
{
    class App : public ListItem
    {
      public:
		App(size_t index, LvObj& parent);

		void setName(std::string_view name) { m_button.setText(name); }
      private:
        static void appClickedEvent(lv_event_t* e);

        Button m_button;
    };
} // namespace UI

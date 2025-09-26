/*
 * Modal.h
 *
 *  Created on: 2025-07-25
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"

namespace UI
{
	template <typename T>
		requires(std::is_base_of_v<LvObj, T>)
	class Modal : public T
	{
	  public:
		Modal(const std::string& name, LvObj& parent, layout_t layout)
			: T(name, parent, layout)
			, m_modalBg("modal_bg", parent, layout_t(0, 0, 100, 100))
		{
			UI_LOCK();

			this->addStyle(Themes::getLvglStyles().card);

			this->setAlign(LV_ALIGN_CENTER, 0, 0);
			this->setParent(m_modalBg);
			m_modalBg.setFlag(LV_OBJ_FLAG_FLOATING, true);

			m_modalBg.addEventCallback(modalBgEventHandler, LV_EVENT_CLICKED, this);

			m_modalBg.addStyle(Themes::getLvglStyles().bg_modal);
		}

		void close()
		{
			if (closeModal(this) || closeScreen(this, false))
			{
				return;
			}
			this->hide();
		}

	  private:
		static void modalBgEventHandler(lv_event_t* e)
		{
			UI_LOCK();

			Modal* modal = (Modal*)lv_event_get_user_data(e);
			if (modal)
			{
				modal->close();
			}
		}

		virtual void onShow() override { m_modalBg.show(true); }
		virtual void onHide() override { m_modalBg.hide(); }

		LvContainer m_modalBg;
	};
} // namespace UI

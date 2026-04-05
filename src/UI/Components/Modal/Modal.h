/*
 * Modal.h
 *
 *  Created on: 2025-07-25
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvContainer.h"
#include "UI/Components/LVGL/LvObj.h"
#include "UI/Components/LVGL/Transitions.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include <concepts>

#ifndef UI_MODAL_CLOSE_BUTTON
#  define UI_MODAL_CLOSE_BUTTON 0
#endif

namespace UI
{
	template <typename T>
		requires(std::is_base_of_v<LvObj, T>)
	class Modal : public T
	{
	  public:
		using modal_base_marker = void; // tag to identify Modal-derived types
		template <typename... Args>
			requires(std::is_constructible_v<T, const std::string&, LvObj&, Args...>)
		Modal(const std::string& name, LvObj& parent, Args&&... args)
			: T(name, parent, std::forward<Args>(args)...)
			, m_modalBg("modal_bg", parent, layout_t(0, 0, 100, 100))
		{
			ZoneScoped;
			UI_LOCK();

			this->addStyle(Themes::getLvglStyles().card);
			this->addStyle(Themes::getLvglStyles().modal);

			this->setAlign(LV_ALIGN_CENTER, 0, 0);
			static_cast<T*>(this)->setParent(m_modalBg); // make sure to call the base setParent
			T::hide();

			m_modalBg.setFlag(LV_OBJ_FLAG_FLOATING, true);
			m_modalBg.addEventCallback(
				[this](lv_event_t*)
				{
					if (!Modal::isBlocking())
						Modal::close();
				},
				LV_EVENT_CLICKED);
			m_modalBg.addStyle(Themes::getLvglStyles().bg_modal);

#if UI_MODAL_CLOSE_BUTTON
			m_closeBtn.setAlign(LV_ALIGN_TOP_RIGHT);
			m_closeBtn.setIcon("close.png");
			m_closeBtn.setText(_("common.close"));
			m_closeBtn.addClickedCallback(
				[this](lv_event_t*)
				{
					if (!Modal::isBlocking())
						Modal::close();
				});
#endif
		}

		void open()
		{
			ZoneScoped;
			openModal(this);
		}
		void close()
		{
			ZoneScoped;
			closeModal(this);
		}

		void setVisible(bool visible)
		{
			ZoneScoped;
			if (visible)
				open();
			else
				close();
		}

		void setParent(LvObj& parent) { m_modalBg.setParent(parent); }

		void setBlocking(bool blocking)
		{
			ZoneScoped;
			m_blocking = blocking;
#if UI_MODAL_CLOSE_BUTTON
			m_closeBtn.setVisible(!blocking);
#endif
		}
		bool isBlocking() const { return m_blocking; }

	  protected:
		/**
		 * @warning private to prevent accidental calls (use this->open() or openModal() instead)
		 */
		void show(bool move_to_front = false) = delete;

		/**
		 * @warning private to prevent accidental calls (use this->close() or closeModal() instead)
		 */
		void hide(bool move_to_front = false) = delete;

		void onShow() override
		{
			ZoneScoped;
			m_modalBg.show(true);
			T::onShow();
			/* Fade in the backdrop and pop the card in with a subtle scale+fade */
			Transitions::fadeIn(m_modalBg, Transitions::animFastDurationMs());
			Transitions::fadeIn(*this);
			Transitions::scaleIn(*this);
		}
		void onHide() override
		{
			ZoneScoped;
			T::onHide();
			/* Fade out the backdrop instead of hiding it instantly */
			Transitions::fadeOutAndHide(m_modalBg, Transitions::animFastDurationMs());
		}

	  private:
		LvContainer m_modalBg;

#if UI_MODAL_CLOSE_BUTTON
		Button m_closeBtn{"modal_close_btn", m_modalBg};
#endif
		bool m_blocking = false;
	};
} // namespace UI

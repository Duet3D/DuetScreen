/*
 * MessageBox.cpp
 *
 *  Created on: 2025-01-29
 *      Author: Andy Everitt
 */

#include "MessageBox.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Styles/Styles.h"
#include "lv_i18n/lv_i18n.h"
#include "lvgl/src/lvgl_private.h"

namespace UI
{

	MessageBox::MessageBox(const std::string& name, LvObj& parent, layout_t layout)
		: LvContainer(name, parent, layout)
	{
		UI_LOCK();
		init();
	}

	MessageBox::~MessageBox()
	{
		UI_LOCK();
		if (!lv_is_initialized())
		{
			return;
		}
		if (m_timers.timeout != nullptr)
		{
			lv_timer_delete(m_timers.timeout);
		}
		if (m_timers.progress != nullptr)
		{
			lv_timer_delete(m_timers.progress);
		}
	}

	void MessageBox::init()
	{
		UI_LOCK();

		addStyle(Themes::getLvglStyles().card);
		m_header.addStyle(Themes::getLvglStyles().bg_color_header);

		// Layout
		setFlexFlow(LV_FLEX_FLOW_COLUMN);

		m_header.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_body.setFlexFlow(LV_FLEX_FLOW_COLUMN);
		m_footer.setFlexFlow(LV_FLEX_FLOW_ROW);

		m_header.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_body.setWidth(LV_PCT(100));
		m_body.setHeight(LV_SIZE_CONTENT);
		m_pad.setWidth(LV_PCT(100));
		m_pad.setFlexGrow(1);
		m_footer.setSize(LV_PCT(100), LV_SIZE_CONTENT);

		// Header

		// Body
		m_bodyTop.setSize(LV_PCT(100), LV_SIZE_CONTENT);
		m_bodyTop.setFlexFlow(LV_FLEX_FLOW_ROW);
		m_text.setFlexGrow(1);
		m_text.setHeight(LV_SIZE_CONTENT);
		m_text.setStyleTextAlign(LV_TEXT_ALIGN_CENTER, 0);
		m_image.setMinWidth(50, LV_PART_MAIN);
		m_image.setMinHeight(50, LV_PART_MAIN);
		m_image.setMaxWidth(300, LV_PART_MAIN);
		m_image.setMaxHeight(300, LV_PART_MAIN);
		m_image.setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);

		// Bottom Container
		m_cancelBtn.setHeight(LV_SIZE_CONTENT);
		m_cancelBtn.setFlexGrow(1);
		m_okBtn.setHeight(LV_SIZE_CONTENT);
		m_okBtn.setFlexGrow(1);
		m_cancelBtn.setText(_("msgbox_cancel"));
		m_okBtn.setText(_("msgbox_ok"));

		m_progress.setSize(LV_PCT(100), 5);

		// Callbacks
		m_okBtn.addClickedCallback(onOkEvent, this);
		m_cancelBtn.addClickedCallback(onCancelEvent, this);

		m_timers.progress = lv_timer_create(onProgressTimer, 50, this);

		// addEventCallback(
		// 	[](lv_event_t* e)
		// 	{
		// 		auto& msgBox = *static_cast<MessageBox*>(lv_event_get_user_data(e));
		// 		lv_coord_t height = msgBox.getHeight();
		// 		lv_coord_t header_height = msgBox.m_header.getHeight();
		// 		lv_coord_t footer_height = msgBox.m_footer.getHeight();

		// 		msgBox.m_body.setHeight(height - header_height - footer_height);
		// 		return;
		// 	},
		// 	LV_EVENT_SIZE_CHANGED,
		// 	this);

		clear();
	}

	void MessageBox::ok()
	{
		UI_LOCK();
		if (m_okCb)
		{
			LOG_INFO("Calling ok callback");
			m_okCb();
		}
		close();
	}

	void MessageBox::cancel()
	{
		UI_LOCK();
		if (m_cancelCb)
		{
			LOG_INFO("Calling cancel callback");
			m_cancelCb();
		}
		close();
	}

	void MessageBox::close()
	{
		UI_LOCK();
		if (getRoot()) // This stops an infrequent segfault when HomePresenter destroys the response message boxes
		{
			closeScreen(this, false);
		}
		if (m_closeCb)
		{
			LOG_INFO("Calling close callback");
			m_closeCb();
		}
	}

	void MessageBox::setTitle(std::string_view text)
	{
		UI_LOCK();
		m_title.setText(text);
	}

	void MessageBox::setText(std::string_view text)
	{
		UI_LOCK();
		m_text.setText(text);
	}

	void MessageBox::setImage(const char* imagePath)
	{
		UI_LOCK();
		m_image.setSrc(imagePath);
		if (m_autoSizeImage)
		{
			m_image.setWidth(m_image.getSrcWidth());
			m_image.setHeight(m_image.getSrcHeight());
		}
		imageVisible(imagePath != nullptr);
	}

	void MessageBox::setImageSize(int32_t width, int32_t height)
	{
		UI_LOCK();
		autoSizeImage(false);
		lv_obj_set_width(m_image, width);
		lv_obj_set_height(m_image, height);
	}

	void MessageBox::setOkBtnText(std::string_view text)
	{
		UI_LOCK();
		m_okBtn.setText(text);
	}

	void MessageBox::setCancelBtnText(std::string_view text)
	{
		UI_LOCK();
		m_cancelBtn.setText(text);
	}

	void MessageBox::clear()
	{
		UI_LOCK();
		setTitle("");
		setText("");
		setImage(nullptr);
		setOkBtnText(_("msgbox_ok"));
		setCancelBtnText(_("msgbox_cancel"));
		okVisible(true);
		cancelVisible(false);
		imageVisible(false);
		progressVisible(false);

		setCancelCallback(nullptr);
		setOkCallback(nullptr);
		setCloseCallback(nullptr);
		setProgressCallback(nullptr);
	}

	void MessageBox::okVisible(bool visible)
	{
		UI_LOCK();
		m_okBtn.setVisible(visible);
		updateVisibility();
	}

	void MessageBox::cancelVisible(bool visible)
	{
		UI_LOCK();
		m_cancelBtn.setVisible(visible);
		updateVisibility();
	}

	void MessageBox::imageVisible(bool visible)
	{
		UI_LOCK();
		m_image.setVisible(visible);
		m_text.setStyleTextAlign(visible ? LV_TEXT_ALIGN_LEFT : LV_TEXT_ALIGN_CENTER, 0);
	}

	void MessageBox::progressVisible(bool visible)
	{
		UI_LOCK();
		m_progress.setVisible(visible);
	}

	void MessageBox::updateVisibility()
	{
		UI_LOCK();
		bool visible = false;
		for (size_t i = 0; i < m_body.getChildCnt(); i++)
		{
			lv_obj_t* child = m_body.getChild(i);
			if (!lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN))
			{
				visible = true;
				break;
			}
		}
		m_body.setVisible(visible);

		visible = false;
		for (size_t i = 0; i < m_footer.getChildCnt(); i++)
		{
			lv_obj_t* child = m_footer.getChild(i);
			if (!lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN))
			{
				visible = true;
				break;
			}
		}
		m_footer.setVisible(visible);
	}

	void MessageBox::setProgress(int percent)
	{
		UI_LOCK();
		lv_bar_set_value(m_progress, percent, LV_ANIM_ON);
	}

	void MessageBox::cancelTimeout()
	{
		UI_LOCK();
		if (m_timers.timeout)
		{
			lv_timer_delete(m_timers.timeout);
			m_timers.timeout = nullptr;
		}
		m_timeout = 0;
	}

	void MessageBox::setTimeout(uint32_t timeout)
	{
		UI_LOCK();
		m_timeout = timeout;
		if (timeout == 0)
		{
			if (m_timers.timeout)
			{
				lv_timer_delete(m_timers.timeout);
				m_timers.timeout = nullptr;
			}
			return;
		}
		m_timers.timeout = lv_timer_create(
			[](lv_timer_t* timer)
			{
				UI_LOCK();
				MessageBox* msgBox = static_cast<MessageBox*>(lv_timer_get_user_data(timer));
				if (msgBox->getRoot())
				{
					msgBox->cancel();
				}
			},
			timeout,
			this);
		lv_timer_set_repeat_count(m_timers.timeout, 1);
	}

	uint32_t MessageBox::getTimeRemaining() const
	{
		UI_LOCK();
		if (m_timers.timeout)
		{
			uint32_t elaps = lv_tick_elaps(m_timers.timeout->last_run);
			if (elaps < m_timeout)
			{
				return m_timeout - elaps;
			}
		}
		return 0;
	}

	uint32_t MessageBox::getTimeOutPercentage() const
	{
		UI_LOCK();
		if (m_timeout == 0)
		{
			return 0u;
		}
		LOG_VERBOSE("Time remaining: {:d}", getTimeRemaining());
		return 100 * getTimeRemaining() / m_timeout;
	}

	void MessageBox::onOkEvent(lv_event_t* e)
	{
		UI_LOCK();
		MessageBox* msgBox = static_cast<MessageBox*>(lv_event_get_user_data(e));
		msgBox->ok();
	}

	void MessageBox::onCancelEvent(lv_event_t* e)
	{
		UI_LOCK();
		MessageBox* msgBox = static_cast<MessageBox*>(lv_event_get_user_data(e));
		msgBox->cancel();
	}

	void MessageBox::onProgressTimer(lv_timer_t* timer)
	{
		UI_LOCK();
		MessageBox* msgBox = static_cast<MessageBox*>(lv_timer_get_user_data(timer));
		if (msgBox->m_progressCb)
		{
			size_t progress = msgBox->m_progressCb(msgBox);
			msgBox->setProgress(progress);
		}
	}
} // namespace UI

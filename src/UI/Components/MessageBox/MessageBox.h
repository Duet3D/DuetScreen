/*
 * MessageBox.h
 *
 *  Created on: 2025-01-29
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/Button/Button.h"
#include "UI/Components/LVGL/LvBar.h"
#include "UI/Components/LVGL/LvImage.h"
#include <functional>
#include <map>
#include <memory>

namespace UI
{

	class MessageBox : public LvContainer
	{

	  public:
		MessageBox(const std::string& name, LvObj& parent, layout_t layout);
		virtual ~MessageBox();

		LvContainer& getHeader() { return m_header; }
		LvContainer& getBody() { return m_body; }
		LvContainer& getFooter() { return m_footer; }
		LvLabel& getTitle() { return m_title; }
		LvLabel& getText() { return m_text; }
		LvImage& getImage() { return m_image; }
		Button& getCancelBtn() { return m_cancelBtn; }
		Button& getOkBtn() { return m_okBtn; }
		LvBar& getProgressBar() { return m_progress; }

		void ok();
		void cancel();
		void close();
		void clear();

		void setTitle(std::string_view title);
		void setText(std::string_view text);

		void setImage(const char* imagePath);
		void autoSizeImage(bool autoSize) { m_autoSizeImage = autoSize; }
		void setImageSize(int32_t width, int32_t height);

		void setOkBtnText(std::string_view text);
		void setCancelBtnText(std::string_view text);
		void setProgress(int percent);

		void setOkCallback(std::function<void()> cb) { m_okCb = cb; }
		void setCancelCallback(std::function<void()> cb) { m_cancelCb = cb; }
		void setCloseCallback(std::function<void()> cb) { m_closeCb = cb; }
		void setProgressCallback(std::function<uint32_t(MessageBox*)> cb) { m_progressCb = cb; }

		void okVisible(bool visible);
		void cancelVisible(bool visible);
		void imageVisible(bool visible);
		void progressVisible(bool visible);
		void updateVisibility();

		void cancelTimeout();
		void setTimeout(uint32_t timeout);
		uint32_t getTimeout() const { return m_timeout; }
		uint32_t getTimeRemaining() const;
		uint32_t getTimeOutPercentage() const;

	  protected:
		static void onOkEvent(lv_event_t* e);
		static void onCancelEvent(lv_event_t* e);
		static void onProgressTimer(lv_timer_t* timer);

		void init();

		LvContainer m_header{"header", getRoot()};
		LvContainer m_body{"body", getRoot()};
		LvContainer m_pad{"pad", getRoot()};
		LvContainer m_footer{"footer", getRoot()};

		// Header
		LvLabel m_title{"title", getHeader()};

		// Body
		LvContainer m_bodyTop{"body_top", getBody()};
		LvLabel m_text{"text", m_bodyTop};
		LvImage m_image{"image", m_bodyTop};

		// Footer
		Button m_cancelBtn{"cancel", getFooter()};
		Button m_okBtn{"ok", getFooter()};
		LvBar m_progress{"progress", getRoot()};

		std::function<void()> m_okCb;
		std::function<void()> m_cancelCb;
		std::function<void()> m_closeCb;
		std::function<uint32_t(MessageBox*)> m_progressCb;

		uint32_t m_timeout = 0;
		bool m_autoSizeImage = true;

		struct
		{
			lv_timer_t* timeout = nullptr;
			lv_timer_t* progress = nullptr;
		} m_timers;
	};
} // namespace UI

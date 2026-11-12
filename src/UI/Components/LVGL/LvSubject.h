/*
 * LvSubject.h
 *
 *  Created on: 2026-01-13
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include <span>

namespace UI
{
#if LV_USE_OBSERVER
	class LvSubject
	{
	  public:
		LvSubject();
		~LvSubject();

		// Observers and XML bindings hold the address of the subject, so it must never move or be copied.
		LvSubject(const LvSubject&) = delete;
		LvSubject& operator=(const LvSubject&) = delete;
		LvSubject(LvSubject&&) = delete;
		LvSubject& operator=(LvSubject&&) = delete;

		bool isInitialized() const { return m_initialized; }

		operator lv_subject_t*() { return &m_subject; }

		void initInt(int32_t value);
		void setInt(int32_t value);
		int32_t getInt() const;
		int32_t getPreviousInt() const;
		void setMinInt(int32_t min_value);
		void setMaxInt(int32_t max_value);

#  if LV_USE_FLOAT
		void initFloat(float value);
		void setFloat(float value);
		float getFloat() const;
		float getPreviousFloat() const;
		void setMinFloat(float min_value);
		void setMaxFloat(float max_value);
#  endif

		void initString(char* buf, char* prev_buf, size_t size, const char* value);
		void copyString(const char* value);
		std::string_view getString() const;
		std::string_view getPreviousString() const;

		void initPointer(void* pointer);
		void setPointer(void* pointer);
		const void* getPointer() const;
		const void* getPreviousPointer() const;

		void initColor(lv_color_t color);
		void setColor(lv_color_t color);
		lv_color_t getColor() const;
		lv_color_t getPreviousColor() const;

		void initGroup(std::span<lv_subject_t*> list);
		lv_subject_t* getGroupElement(int32_t index) const;

		lv_observer_t* addObserver(lv_observer_cb_t observer_cb, void* user_data = nullptr);
		lv_observer_t* addObserverObj(lv_observer_cb_t observer_cb, LvObj& obj, void* user_data = nullptr);
		lv_observer_t* addObserverWithTarget(lv_observer_cb_t observer_cb, void* target, void* user_data = nullptr);
		void removeObserver(lv_observer_t* observer);

		void deinit();

        void notify();

	  private:
		lv_subject_t m_subject;

        bool m_initialized{false};
	};
#endif
} // namespace UI
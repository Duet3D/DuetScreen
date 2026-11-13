/*
 * LvAnim.h
 *
 *  Created on: 2025-07-24
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvObj.h"

namespace UI
{
	class LvAnim
	{
	  public:
		LvAnim();
		operator lv_anim_t*() { return &m_anim; }

		/* Mandatory settings */

		void setVar(void* var);
		void setExecCb(lv_anim_exec_xcb_t exec_cb);
		void setDuration(uint32_t duration);
		void setValues(int32_t start, int32_t end);

		/* Control */

		LvAnim* start();
		void resume();
		void pause();
		void pauseFor(uint32_t ms);
		bool isPaused() const;

		/* Optional settings */

		void setCustomExecCb(lv_anim_custom_exec_cb_t exec_cb);
		void setDelay(uint32_t delay);
		void setPathCb(lv_anim_path_cb_t path_cb);
		void setStartCb(lv_anim_start_cb_t start_cb);
		void setGetValueCb(lv_anim_get_value_cb_t get_value_cb);
		void setCompletedCb(lv_anim_completed_cb_t completed_cb);
		void setDeletedCb(lv_anim_deleted_cb_t deleted_cb);
		void setReverseDuration(uint32_t duration);
		void setReverseDelay(uint32_t delay);
		void setRepeatCount(uint32_t cnt);
		void setRepeatDelay(uint32_t delay);
		void setEarlyApply(bool en);
		void setUserData(void* userData);
		void setBezier3Path(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

		/* Getters */
		void* getVar() const;
		uint32_t getDelay() const;
		uint32_t getPlaytime() const;
		uint32_t getTime() const;
		uint32_t getRepeatCount() const;
		void* getUserData() const;

	  private:
		lv_anim_t m_anim;
	};
} // namespace UI

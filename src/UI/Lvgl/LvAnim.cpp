/*
 * LvAnim.cpp
 *
 *  Created on: 2025-07-24
 *      Author: Andy Everitt
 */

#include "LvAnim.h"
#include "Debug.h"

namespace UI
{
	LvAnim::LvAnim()
	{
		UI_LOCK();
		lv_anim_init(&m_anim);
	}

	/**
	 * Set a variable to animate
	 * @param var   pointer to a variable to animate
	 */
	void LvAnim::setVar(void* var)
	{
		UI_LOCK();
		lv_anim_set_var(&m_anim, var);
	}

	/**
	 * Set a function to animate `var`
	 * @param exec_cb   a function to execute during animation
	 *                  LVGL's built-in functions can be used.
	 *                  E.g. lv_obj_set_x
	 */
	void LvAnim::setExecCb(lv_anim_exec_xcb_t exec_cb)
	{
		UI_LOCK();
		lv_anim_set_exec_cb(&m_anim, exec_cb);
	}

	/**
	 * Set the duration of an animation
	 * @param duration  duration of the animation in milliseconds
	 */
	void LvAnim::setDuration(uint32_t duration)
	{
		UI_LOCK();
		lv_anim_set_duration(&m_anim, duration);
	}

	/**
	 * Set a delay before starting the animation
	 * @param delay     delay before the animation in milliseconds
	 */
	void LvAnim::setDelay(uint32_t delay)
	{
		UI_LOCK();
		lv_anim_set_delay(&m_anim, delay);
	}

	/**
	 * Resumes a paused animation
	 */
	void LvAnim::resume()
	{
		UI_LOCK();
		lv_anim_resume(&m_anim);
	}

	/**
	 * Pauses the animation
	 */
	void LvAnim::pause()
	{
		UI_LOCK();
		lv_anim_pause(&m_anim);
	}

	/**
	 * Pauses the animation for ms milliseconds
	 * @param ms        the pause time in milliseconds
	 */
	void LvAnim::pauseFor(uint32_t ms)
	{
		UI_LOCK();
		lv_anim_pause_for(&m_anim, ms);
	}

	/**
	 * Check if the animation is paused
	 * @return          true if the animation is paused else false
	 */
	bool LvAnim::isPaused() const
	{
		UI_LOCK();
		return lv_anim_is_paused(const_cast<lv_anim_t*>(&m_anim));
	}

	/**
	 * Set the start and end values of an animation
	 * @param start     the start value
	 * @param end       the end value
	 */
	void LvAnim::setValues(int32_t start, int32_t end)
	{
		UI_LOCK();
		lv_anim_set_values(&m_anim, start, end);
	}

	/**
	 * Similar to `lv_anim_set_exec_cb` but `lv_anim_custom_exec_cb_t` receives
	 * `lv_anim_t * ` as its first parameter instead of `void *`.
	 * This function might be used when LVGL is bound to other languages because
	 * it's more consistent to have `lv_anim_t *` as first parameter.
	 * @param exec_cb   a function to execute.
	 */
	void LvAnim::setCustomExecCb(lv_anim_custom_exec_cb_t exec_cb)
	{
		UI_LOCK();
		lv_anim_set_custom_exec_cb(&m_anim, exec_cb);
	}

	/**
	 * Set the path (curve) of the animation.
	 * @param path_cb a function to set the current value of the animation.
	 */
	void LvAnim::setPathCb(lv_anim_path_cb_t path_cb)
	{
		UI_LOCK();
		lv_anim_set_path_cb(&m_anim, path_cb);
	}

	/**
	 * Set a function call when the animation really starts (considering `delay`)
	 * @param start_cb  a function call when the animation starts
	 */
	void LvAnim::setStartCb(lv_anim_start_cb_t start_cb)
	{
		UI_LOCK();
		lv_anim_set_start_cb(&m_anim, start_cb);
	}

	/**
	 * Set a function to use the current value of the variable and make start and end value
	 * relative to the returned current value.
	 * @param get_value_cb  a function call when the animation starts
	 */
	void LvAnim::setGetValueCb(lv_anim_get_value_cb_t get_value_cb)
	{
		UI_LOCK();
		lv_anim_set_get_value_cb(&m_anim, get_value_cb);
	}

	/**
	 * Set a function call when the animation is completed
	 * @param completed_cb  a function call when the animation is fully completed
	 */
	void LvAnim::setCompletedCb(lv_anim_completed_cb_t completed_cb)
	{
		UI_LOCK();
		lv_anim_set_completed_cb(&m_anim, completed_cb);
	}

	/**
	 * Set a function call when the animation is deleted.
	 * @param deleted_cb  a function call when the animation is deleted
	 */
	void LvAnim::setDeletedCb(lv_anim_deleted_cb_t deleted_cb)
	{
		UI_LOCK();
		lv_anim_set_deleted_cb(&m_anim, deleted_cb);
	}

	/**
	 * Make the animation to play back to when the forward direction is ready
	 * @param duration  duration of playback animation in milliseconds. 0: disable playback
	 */
	void LvAnim::setReverseDuration(uint32_t duration)
	{
		UI_LOCK();
		lv_anim_set_reverse_duration(&m_anim, duration);
	}

	/**
	 * Make the animation to play back to when the forward direction is ready
	 * @param delay     delay in milliseconds before starting the playback animation.
	 */
	void LvAnim::setReverseDelay(uint32_t delay)
	{
		UI_LOCK();
		lv_anim_set_reverse_delay(&m_anim, delay);
	}

	/**
	 * Make the animation repeat itself.
	 * @param cnt       repeat count or `LV_ANIM_REPEAT_INFINITE` for infinite repetition. 0: to disable repetition.
	 */
	void LvAnim::setRepeatCount(uint32_t cnt)
	{
		UI_LOCK();
		lv_anim_set_repeat_count(&m_anim, cnt);
	}

	/**
	 * Set a delay before repeating the animation.
	 * @param delay     delay in milliseconds before repeating the animation.
	 */
	void LvAnim::setRepeatDelay(uint32_t delay)
	{
		UI_LOCK();
		lv_anim_set_repeat_delay(&m_anim, delay);
	}
	/**
	 * Set a whether the animation's should be applied immediately or only when the delay expired.
	 * @param en        true: apply the start value immediately in `lv_anim_start`;
	 *                  false: apply the start value only when `delay` ms is elapsed and the animations really starts
	 */
	void LvAnim::setEarlyApply(bool en)
	{
		UI_LOCK();
		lv_anim_set_early_apply(&m_anim, en);
	}

	/**
	 * Set the custom user data field of the animation.
	 * @param user_data   pointer to the new user_data.
	 */
	void LvAnim::setUserData(void* userData)
	{
		UI_LOCK();
		lv_anim_set_user_data(&m_anim, userData);
	}
	/**
	 * Set parameter for cubic bezier path
	 * @param x1        first control point X
	 * @param y1        first control point Y
	 * @param x2        second control point X
	 * @param y2        second control point Y
	 */
	void LvAnim::setBezier3Path(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
	{
		UI_LOCK();
		lv_anim_set_bezier3_param(&m_anim, x1, y1, x2, y2);
	}

	/**
	 * Create an animation
	 * @return          pointer to the created animation (different from the `a` parameter)
	 */
	LvAnim* LvAnim::start()
	{
		UI_LOCK();
		return (LvAnim*)lv_anim_start(&m_anim);
	}

	/**
	 * Get the variable to animate
	 * @return         pointer to the variable to animate
	 */
	void* LvAnim::getVar() const
	{
		return m_anim.var;
	}

	/**
	 * Get a delay before starting the animation
	 * @return delay before the animation in milliseconds
	 */
	uint32_t LvAnim::getDelay() const
	{
		UI_LOCK();
		return lv_anim_get_delay(&m_anim);
	}

	/**
	 * Get the time used to play the animation.
	 * @return the play time in milliseconds.
	 */
	uint32_t LvAnim::getPlaytime() const
	{
		UI_LOCK();
		return lv_anim_get_playtime(&m_anim);
	}

	/**
	 * Get the duration of an animation
	 * @return the duration of the animation in milliseconds
	 */
	uint32_t LvAnim::getTime() const
	{
		UI_LOCK();
		return lv_anim_get_time(&m_anim);
	}

	/**
	 * Get the repeat count of the animation.
	 * @return the repeat count or `LV_ANIM_REPEAT_INFINITE` for infinite repetition. 0: disabled repetition.
	 */
	uint32_t LvAnim::getRepeatCount() const
	{
		UI_LOCK();
		return lv_anim_get_repeat_count(&m_anim);
	}

	/**
	 * Get the user_data field of the animation
	 * @return  the pointer to the custom user_data of the animation
	 */
	void* LvAnim::getUserData() const
	{
		UI_LOCK();
		return lv_anim_get_user_data(&m_anim);
	}
} // namespace UI

/*
 * Transitions.cpp
 *
 *  Created on: 2026-04-05
 *      Author: Andy Everitt
 */

#include "Transitions.h"
#include "LvAnim.h"
#include "UI/Styles/Styles.h"
#include "lvgl/src/misc/lv_math.h" // LV_BEZIER_VAL_FLOAT, LV_BEZIER_VAL_MAX
#include "utils/StorageHelper.h"

namespace UI::Transitions
{
	namespace
	{
		static bool animationsEnabled()
		{
			return StorageHelper::getData(ID_UI_ANIMATIONS_ENABLED);
		}

		static uint32_t themeAnimDurationMs()
		{
			lv_style_value_t v;
			if (Themes::getLvglStyles().anim.getProp(LV_STYLE_ANIM_DURATION, &v) == LV_STYLE_RES_FOUND)
				return static_cast<uint32_t>(v.num);
			return 200;
		}

		static uint32_t themeAnimFastDurationMs()
		{
			lv_style_value_t v;
			if (Themes::getLvglStyles().anim_fast.getProp(LV_STYLE_ANIM_DURATION, &v) == LV_STYLE_RES_FOUND)
				return static_cast<uint32_t>(v.num);
			return 120;
		}

		/* ── Shared animation callbacks ─────────────────────────────── */

		/* Drives LV_STYLE_OPA each animation tick. The var holds lv_obj_t* so
		 * LVGL can automatically cancel the animation when the object is deleted. */
		static void opaAnimCb(void* var, int32_t v)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(var));
			if (obj == nullptr)
				return;
			lv_style_value_t sv;
			sv.num = v;
			obj->setLocalStyleProp(LV_STYLE_OPA, sv);
		}

		/* Drives transform scale each animation tick. lv_obj_set_style_transform_scale
		 * updates both axes in one call; no single LvObj wrapper exists for it yet. */
		static void scaleAnimCb(void* var, int32_t v)
		{
			lv_obj_set_style_transform_scale(static_cast<lv_obj_t*>(var), v, LV_PART_MAIN);
		}

		static void fadeInCompletedCb(lv_anim_t* a)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(a->var));
			if (obj == nullptr)
				return;
			obj->removeLocalStyleProp(LV_STYLE_OPA);
		}

		static void fadeOutHideCompletedCb(lv_anim_t* a)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(a->var));
			if (obj == nullptr)
				return;
			obj->removeLocalStyleProp(LV_STYLE_OPA);
			obj->setFlag(LV_OBJ_FLAG_HIDDEN, true);
		}

		static void scaleInCompletedCb(lv_anim_t* a)
		{
			LvObj* obj = LvObj::fromPtr(static_cast<lv_obj_t*>(a->var));
			if (obj == nullptr)
				return;
			obj->removeLocalStyleProp(LV_STYLE_TRANSFORM_SCALE_X);
			obj->removeLocalStyleProp(LV_STYLE_TRANSFORM_SCALE_Y);
			obj->removeLocalStyleProp(LV_STYLE_TRANSFORM_PIVOT_X);
			obj->removeLocalStyleProp(LV_STYLE_TRANSFORM_PIVOT_Y);
		}
	} // namespace

	uint32_t animDurationMs()
	{
		return themeAnimDurationMs();
	}

	uint32_t animFastDurationMs()
	{
		return themeAnimFastDurationMs();
	}

	void fadeIn(LvObj& obj, uint32_t durationMs, uint32_t delayMs)
	{
		if (!animationsEnabled())
		{
			obj.removeLocalStyleProp(LV_STYLE_OPA);
			return;
		}
		if (durationMs == 0)
			durationMs = themeAnimDurationMs();

		LvAnim anim;
		anim.setVar(obj.getRootPtr());
		anim.setValues(LV_OPA_TRANSP, LV_OPA_COVER);
		anim.setExecCb(opaAnimCb);
		anim.setDuration(durationMs);
		anim.setDelay(delayMs);
		anim.setPathCb(lv_anim_path_ease_out);
		anim.setCompletedCb(fadeInCompletedCb);
		anim.start();
	}

	void scaleIn(LvObj& obj, uint32_t durationMs, int32_t startScale, uint32_t delayMs)
	{
		if (!animationsEnabled())
		{
			obj.removeLocalStyleProp(LV_STYLE_TRANSFORM_SCALE_X);
			obj.removeLocalStyleProp(LV_STYLE_TRANSFORM_SCALE_Y);
			obj.removeLocalStyleProp(LV_STYLE_TRANSFORM_PIVOT_X);
			obj.removeLocalStyleProp(LV_STYLE_TRANSFORM_PIVOT_Y);
			return;
		}
		if (durationMs == 0)
			durationMs = themeAnimDurationMs();

		/* Centre the transform pivot so the object zooms in symmetrically */
		lv_style_value_t pct50;
		pct50.num = lv_pct(50);
		obj.setLocalStyleProp(LV_STYLE_TRANSFORM_PIVOT_X, pct50);
		obj.setLocalStyleProp(LV_STYLE_TRANSFORM_PIVOT_Y, pct50);

		LvAnim anim;
		anim.setVar(obj.getRootPtr());
		anim.setValues(startScale, LV_SCALE_NONE);
		anim.setExecCb(scaleAnimCb);
		anim.setDuration(durationMs);
		anim.setDelay(delayMs);
		/* ease-out-quart cubic-bezier: (0.25, 1.0, 0.5, 1.0) */
		anim.setPathCb(lv_anim_path_custom_bezier3);
		anim.setBezier3Path(static_cast<int16_t>(LV_BEZIER_VAL_FLOAT(0.25)),
							static_cast<int16_t>(LV_BEZIER_VAL_FLOAT(1.0)),
							static_cast<int16_t>(LV_BEZIER_VAL_FLOAT(0.5)),
							static_cast<int16_t>(LV_BEZIER_VAL_FLOAT(1.0)));
		anim.setCompletedCb(scaleInCompletedCb);
		anim.start();
	}

	void fadeOutAndHide(LvObj& obj, uint32_t durationMs, uint32_t delayMs)
	{
		if (!animationsEnabled())
		{
			obj.removeLocalStyleProp(LV_STYLE_OPA);
			obj.setFlag(LV_OBJ_FLAG_HIDDEN, true);
			return;
		}
		if (durationMs == 0)
			durationMs = themeAnimFastDurationMs();

		LvAnim anim;
		anim.setVar(obj.getRootPtr());
		anim.setValues(LV_OPA_COVER, LV_OPA_TRANSP);
		anim.setExecCb(opaAnimCb);
		anim.setDuration(durationMs);
		anim.setDelay(delayMs);
		anim.setPathCb(lv_anim_path_ease_in);
		anim.setCompletedCb(fadeOutHideCompletedCb);
		anim.start();
	}

} // namespace UI::Transitions

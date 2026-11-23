/*
 * Transitions.h
 *
 *  Created on: 2026-04-05
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Lvgl/LvObj.h"
#include <cstdint>

namespace UI::Transitions
{
	/**
	 * @brief Returns the normal animation duration from the active theme's `anim` style.
	 *
	 * Falls back to 200 ms if the theme does not define one.
	 */
	[[nodiscard]] uint32_t animDurationMs();

	/**
	 * @brief Returns the fast animation duration from the active theme's `anim_fast` style.
	 *
	 * Falls back to 120 ms if the theme does not define one.
	 */
	[[nodiscard]] uint32_t animFastDurationMs();

	/**
	 * @brief Fade an object in from fully transparent to fully opaque.
	 *
	 * Uses an ease-out curve so the animation decelerates naturally. A local `LV_STYLE_OPA`
	 * property is applied to drive the animation and removed on completion so no permanent
	 * local style is left behind.
	 *
	 * @param obj        LVGL object to animate. Must not be nullptr.
	 * @param durationMs Duration in ms. Pass 0 (default) to use the theme's `anim` duration.
	 * @param delayMs    Delay before the animation starts.
	 */
	void fadeIn(LvObj& obj, uint32_t durationMs = 0, uint32_t delayMs = 0);

	/**
	 * @brief Animate an object scaling from `startScale` up to `LV_SCALE_NONE` (100%).
	 *
	 * The transform pivot is set to the object's centre so the scale appears to zoom-in
	 * symmetrically. Uses an ease-out curve. Local transform-scale and pivot style
	 * properties are removed on completion.
	 *
	 * @param obj        Object to animate.
	 * @param durationMs Duration in ms. Pass 0 (default) to use the theme's `anim` duration.
	 * @param startScale Starting scale value (256 == 100%, so 230 ≈ 90%). Default is 224 (≈87.5%).
	 * @param delayMs    Delay before the animation starts.
	 */
	void scaleIn(LvObj& obj, uint32_t durationMs = 0, int32_t startScale = 224, uint32_t delayMs = 0);

	/**
	 * @brief Fade an object out then hide it.
	 *
	 * Animates `LV_STYLE_OPA` from `LV_OPA_COVER` to `LV_OPA_TRANSP` using an ease-in curve,
	 * then sets `LV_OBJ_FLAG_HIDDEN` and removes the local opa style on completion so the
	 * object returns to its unstyled opacity when shown again.
	 *
	 * @param obj        Object to animate.
	 * @param durationMs Duration in ms. Pass 0 (default) to use the theme's `anim_fast` duration.
	 * @param delayMs    Delay before the animation starts.
	 */
	void fadeOutAndHide(LvObj& obj, uint32_t durationMs = 0, uint32_t delayMs = 0);

	/**
	 * @brief Replace `from` with `to`, two pages in the same place: `to` slides in across `width` while `from` slides
	 * out, or, with animations off, swaps at once.
	 *
	 * `direction` 1 brings `to` in from the right (going forward, e.g. to a later tab or a sub-page), -1 from the left
	 * (going back). `from` ends hidden and `to` shown and in front, both where they started. A slide in flight on either
	 * is cut short first. Uses the theme's `anim` duration.
	 *
	 * @param from      The page shown now, or nullptr for none (then `to` is shown at once).
	 * @param to        The page to show.
	 * @param direction 1 or -1, as above.
	 * @param width     How far the pages slide: the width of the area they share.
	 * @param animate   False to swap at once even with animations on.
	 */
	void slide(LvObj* from, LvObj& to, int32_t direction, lv_coord_t width, bool animate = true);

	/**
	 * @brief Cut short a slide (see slide()) of `obj`, leaving it where it started; its hidden flag is left as it is.
	 */
	void stopSlide(LvObj& obj);
} // namespace UI::Transitions

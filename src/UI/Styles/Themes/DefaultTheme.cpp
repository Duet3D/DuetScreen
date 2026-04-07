/*
 * DefaultTheme.cpp
 *
 *  Created on: 2025-06-02
 *      Author: Andy Everitt
 */

#include "DefaultTheme.h"
#include "Debug.h"
#include <algorithm>

namespace UI::Themes
{
#define TRANSITION_TIME 80
#define BORDER_WIDTH 2
#define OUTLINE_WIDTH 3
#define SHADOW_WIDTH 2
#define SHADOW_OFFSET 3

#define RADIUS_DEFAULT 5

#define PAD_LARGE 10
#define PAD_DEF 5
#define PAD_SMALL 2
#define PAD_TINY 1

	static constexpr float s_minColorChroma = 0.1f;

	ThemeColors createThemeColors(ColorCtx ctx)
	{
		ThemeColors colors;
		ctx.primaryHue = std::clamp<uint16_t>(ctx.primaryHue, 0u, 360u);
		ctx.secondaryHue = std::clamp<uint16_t>(ctx.secondaryHue, 0, 360u);
		ctx.chroma = std::clamp<float>(ctx.chroma, 0.0f, 0.2f);

		const float bgChroma = ctx.chroma / 2;
		const float colorChroma = std::max(s_minColorChroma, ctx.chroma);
		static float mutedDiff = 0.2f;

		colors.bg_dark = Color(ctx.darkMode ? 0.1f : 0.92f, bgChroma, ctx.primaryHue);
		colors.bg = Color(ctx.darkMode ? 0.15f : 0.96f, bgChroma, ctx.primaryHue);
		colors.bg_light = Color(ctx.darkMode ? 0.25f : 1.0f, bgChroma, ctx.primaryHue);

		const float colorL = ctx.darkMode ? 0.45f : 0.7f;
		const float mutedL = colorL - mutedDiff;

		colors.primary = Color(colorL, colorChroma, ctx.primaryHue);
		colors.primary_muted = Color(mutedL, colorChroma, ctx.primaryHue);
		colors.secondary = Color(colorL, colorChroma, ctx.secondaryHue);
		colors.secondary_muted = Color(mutedL, colorChroma, ctx.secondaryHue);

		colors.text = Color(ctx.darkMode ? 0.96f : 0.15f, ctx.chroma, ctx.primaryHue);
		colors.text_muted = Color(ctx.darkMode ? 0.82f : 0.25f, ctx.chroma, ctx.primaryHue);
		colors.text_header = Color(ctx.darkMode ? 1.0f : 0.0f, ctx.chroma, ctx.primaryHue);

		colors.border = Color(ctx.darkMode ? 0.40f : 0.6f, ctx.chroma, ctx.primaryHue);
		colors.border_muted = Color(ctx.darkMode ? 0.30f : 0.7f, ctx.chroma, ctx.primaryHue);
		colors.highlight = Color(ctx.darkMode ? 0.70f : 1.0f, ctx.chroma, ctx.primaryHue);
		colors.shadow = Color(ctx.darkMode ? 0.2f : 0.4f, bgChroma, ctx.primaryHue);

		colors.success = Color(ctx.darkMode ? 0.6f : 0.8f, std::max(0.15f, ctx.chroma), 144);
		colors.warning = Color(ctx.darkMode ? 0.75f : 0.85f, std::max(0.17f, ctx.chroma), 78);
		colors.error = Color(ctx.darkMode ? 0.6f : 0.8f, std::max(0.2f, ctx.chroma), 27);

		if (ctx.customizer)
		{
			ctx.customizer(colors, ctx);
		}
		return colors;
	}

	void DefaultTheme::updateColors(const ThemeColors& colors)
	{
		m_colors = colors;
		init();
	}

	void DefaultTheme::onInit()
	{
		Theme::onInit();

		LvglStyles& lvgl = getLvglStyles();
		ComponentStyles& components = getComponentStyles();
		const Fonts& fonts = getFonts(); /* These are the static fonts which the theme will copy the fonts too */

		/* Backgrounds */

		lv_style_set_bg_color(lvgl.bg_dark, m_colors.bg_dark);
		lv_style_set_bg_opa(lvgl.bg_dark, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_dark, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg, m_colors.bg);
		lv_style_set_bg_opa(lvgl.bg, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_light, m_colors.bg_light);
		lv_style_set_bg_opa(lvgl.bg_light, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_light, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_primary, m_colors.primary);
		lv_style_set_bg_opa(lvgl.bg_color_primary, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_primary, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_primary_muted, m_colors.primary_muted);
		lv_style_set_bg_opa(lvgl.bg_color_primary_muted, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_primary_muted, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_secondary, m_colors.secondary);
		lv_style_set_bg_opa(lvgl.bg_color_secondary, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_secondary, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_secondary_muted, m_colors.secondary_muted);
		lv_style_set_bg_opa(lvgl.bg_color_secondary_muted, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_secondary_muted, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_success, m_colors.success);
		lv_style_set_bg_opa(lvgl.bg_color_success, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_success, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_warning, m_colors.warning);
		lv_style_set_bg_opa(lvgl.bg_color_warning, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_warning, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_error, m_colors.error);
		lv_style_set_bg_opa(lvgl.bg_color_error, LV_OPA_COVER);
		lv_style_set_bg_grad_dir(lvgl.bg_color_error, LV_GRAD_DIR_NONE);

		lv_style_set_bg_color(lvgl.bg_color_header, lv_color_white());
		lv_style_set_bg_opa(lvgl.bg_color_header, LV_OPA_10);

		lv_style_set_bg_color(lvgl.bg_color_list_item, m_colors.bg);
		lv_style_set_bg_opa(lvgl.bg_color_list_item, LV_OPA_COVER);
		lv_style_set_bg_grad_color(lvgl.bg_color_list_item, m_colors.bg_light);

		lv_style_set_bg_color(lvgl.bg_modal, m_colors.bg);
		lv_style_set_bg_opa(lvgl.bg_modal, LV_OPA_70);

		/* Text */

		lv_style_set_text_color(lvgl.text, m_colors.text);
		lv_style_set_text_font(lvgl.text, &fonts.normal);

		lv_style_set_text_color(lvgl.text_muted, m_colors.text_muted);
		lv_style_set_text_font(lvgl.text_muted, &fonts.subdued);

		lv_style_set_text_color(lvgl.text_emphasis, m_colors.text);
		lv_style_set_text_font(lvgl.text_emphasis, &fonts.emphasis);

		lv_style_set_text_color(lvgl.text_header, m_colors.text_header);
		lv_style_set_text_font(lvgl.text_header, &fonts.header);

		lv_style_set_text_color(lvgl.text_warning, m_colors.error);
		lv_style_set_text_font(lvgl.text_warning, &fonts.emphasis);

		lv_style_set_text_line_space(lvgl.line_space_large, LV_DPX_CALC(lv_display_get_dpi(NULL), 20));

		lv_style_set_text_align(lvgl.text_align_center, LV_TEXT_ALIGN_CENTER);

		/* Borders */

		lv_style_set_border_color(lvgl.border, m_colors.border);
		lv_style_set_border_width(lvgl.border, BORDER_WIDTH);
		lv_style_set_border_opa(lvgl.border, LV_OPA_COVER);
		lv_style_set_border_side(lvgl.border, LV_BORDER_SIDE_FULL);

		lv_style_set_border_color(lvgl.border_muted, m_colors.border_muted);
		lv_style_set_border_width(lvgl.border_muted, BORDER_WIDTH);
		lv_style_set_border_opa(lvgl.border_muted, LV_OPA_COVER);
		lv_style_set_border_side(lvgl.border_muted, LV_BORDER_SIDE_FULL);

		lv_style_set_border_color(lvgl.border_highlight, m_colors.highlight);
		lv_style_set_border_width(lvgl.border_highlight, BORDER_WIDTH);
		lv_style_set_border_opa(lvgl.border_highlight, LV_OPA_COVER);
		lv_style_set_border_side(lvgl.border_highlight, LV_BORDER_SIDE_FULL);

		lv_style_set_border_color(lvgl.border_color_primary, m_colors.primary);
		lv_style_set_border_width(lvgl.border_color_primary, BORDER_WIDTH);
		lv_style_set_border_opa(lvgl.border_color_primary, LV_OPA_COVER);
		lv_style_set_border_side(lvgl.border_color_primary, LV_BORDER_SIDE_FULL);

		lv_style_set_border_color(lvgl.border_color_secondary, m_colors.secondary);
		lv_style_set_border_width(lvgl.border_color_secondary, BORDER_WIDTH);
		lv_style_set_border_opa(lvgl.border_color_secondary, LV_OPA_COVER);
		lv_style_set_border_side(lvgl.border_color_secondary, LV_BORDER_SIDE_FULL);

		/* Outlines */

		lv_style_set_outline_color(lvgl.outline_primary, m_colors.primary);
		lv_style_set_outline_width(lvgl.outline_primary, OUTLINE_WIDTH);
		lv_style_set_outline_pad(lvgl.outline_primary, OUTLINE_WIDTH);
		lv_style_set_outline_opa(lvgl.outline_primary, LV_OPA_50);

		lv_style_set_outline_color(lvgl.outline_secondary, m_colors.secondary);
		lv_style_set_outline_width(lvgl.outline_secondary, OUTLINE_WIDTH);
		lv_style_set_outline_opa(lvgl.outline_secondary, LV_OPA_50);

		/* Scrollbar */

		lv_style_set_bg_color(lvgl.scrollbar, m_colors.bg_light);
		lv_style_set_border_color(lvgl.scrollbar, m_colors.border);
		lv_style_set_border_width(lvgl.scrollbar, 1);
		lv_style_set_radius(lvgl.scrollbar, LV_RADIUS_CIRCLE);
		lv_style_set_pad_all(lvgl.scrollbar, LV_DPX_CALC(lv_display_get_dpi(NULL), 7));
		lv_style_set_width(lvgl.scrollbar, LV_DPX_CALC(lv_display_get_dpi(NULL), 5));
		lv_style_set_bg_opa(lvgl.scrollbar, LV_OPA_40);

		lv_style_set_bg_opa(lvgl.scrollbar_scrolled, LV_OPA_COVER);

		/* Base */

		lv_style_set_bg_opa(lvgl.base, LV_OPA_TRANSP);
		lv_style_set_border_post(lvgl.base, true);

		/* Screen */

		lv_style_set_bg_color(lvgl.screen, m_colors.bg_dark);
		lv_style_set_bg_opa(lvgl.screen, LV_OPA_COVER);

		/* Card */

		lv_style_set_radius(lvgl.card, RADIUS_DEFAULT);
		lv_style_set_pad_all(lvgl.card, PAD_DEF);
		lv_style_set_pad_row(lvgl.card, PAD_SMALL);
		lv_style_set_pad_column(lvgl.card, PAD_SMALL);

		lv_style_set_bg_opa(lvgl.card, LV_OPA_COVER);
		lv_style_set_bg_color(lvgl.card, m_colors.bg);
		// lv_style_set_bg_grad_color(lvgl.card, m_colors.bg);
		// lv_style_set_bg_grad_dir(lvgl.card, LV_GRAD_DIR_VER);

		lv_style_set_border_color(lvgl.card, m_colors.border);
		lv_style_set_border_width(lvgl.card, BORDER_WIDTH);
		lv_style_set_border_opa(lvgl.card, LV_OPA_COVER);
		lv_style_set_border_side(lvgl.card, LV_BORDER_SIDE_FULL);

		/* Shadow */
		lv_style_set_radius(lvgl.shadow_raised, 3);
		lv_style_set_border_width(lvgl.shadow_raised, 5);
		lv_style_set_border_color(lvgl.shadow_raised, lv_color_white());
		lv_style_set_border_opa(lvgl.shadow_raised, LV_OPA_30);
		lv_style_set_border_side(lvgl.shadow_raised, LV_BORDER_SIDE_TOP);
		lv_style_set_shadow_color(lvgl.shadow_raised, lv_color_black());
		lv_style_set_shadow_offset_y(lvgl.shadow_raised, 5);
		lv_style_set_shadow_opa(lvgl.shadow_raised, LV_OPA_30);
		lv_style_set_shadow_width(lvgl.shadow_raised, 10);

		lv_style_set_shadow_width(lvgl.shadow_lowered, 6);
		lv_style_set_shadow_color(lvgl.shadow_lowered, lv_color_black());
		lv_style_set_shadow_opa(lvgl.shadow_lowered, LV_OPA_30);
		lv_style_set_shadow_offset_y(lvgl.shadow_lowered, -3);

		/* Button */

		lv_style_set_radius(lvgl.btn, RADIUS_DEFAULT);

		lv_style_set_bg_opa(lvgl.btn, LV_OPA_COVER);
		lv_style_set_bg_color(lvgl.btn, m_colors.primary);
		// lv_style_set_bg_grad_color(lvgl.btn, m_colors.primary_muted);
		// lv_style_set_bg_grad_dir(lvgl.btn, LV_GRAD_DIR_VER);

		lv_style_set_bg_grad_dir(lvgl.btn_checked, LV_GRAD_DIR_NONE);
		lv_style_merge(lvgl.btn_checked, lvgl.border_highlight);

		lv_style_set_bg_opa(lvgl.actionBtn, LV_OPA_COVER);
		lv_style_set_bg_color(lvgl.actionBtn, m_colors.secondary);
		// lv_style_set_bg_grad_color(lvgl.actionBtn, m_colors.secondary_muted);
		// lv_style_set_bg_grad_dir(lvgl.actionBtn, LV_GRAD_DIR_VER);

		/* Icons */
		lv_style_set_recolor(lvgl.icon_recolor, m_colors.text);
		lv_style_set_recolor_opa(lvgl.icon_recolor, LV_OPA_COVER);

		/* Modifiers */

		lv_style_set_recolor(lvgl.pressed, lv_color_black());
		lv_style_set_recolor_opa(lvgl.pressed, 35);

		lv_style_set_recolor(lvgl.disabled, m_colors.bg);
		lv_style_set_recolor_opa(lvgl.disabled, LV_OPA_50);

		lv_style_set_clip_corner(lvgl.clip_corner, true);
		lv_style_set_border_post(lvgl.clip_corner, true);

		/* Padding */

		lv_style_set_pad_all(lvgl.pad_large, PAD_LARGE);
		lv_style_set_pad_row(lvgl.pad_large, PAD_LARGE);
		lv_style_set_pad_column(lvgl.pad_large, PAD_LARGE);

		lv_style_set_pad_all(lvgl.pad_normal, PAD_DEF);
		lv_style_set_pad_row(lvgl.pad_normal, PAD_DEF);
		lv_style_set_pad_column(lvgl.pad_normal, PAD_DEF);

		lv_style_set_pad_all(lvgl.pad_small, PAD_SMALL);
		lv_style_set_pad_gap(lvgl.pad_small, PAD_SMALL);

		lv_style_set_pad_row(lvgl.pad_gap, LV_DPX_CALC(lv_display_get_dpi(NULL), 10));
		lv_style_set_pad_column(lvgl.pad_gap, LV_DPX_CALC(lv_display_get_dpi(NULL), 10));

		lv_style_set_pad_all(lvgl.pad_base, PAD_DEF);
		lv_style_set_pad_row(lvgl.pad_base, PAD_DEF);
		lv_style_set_pad_column(lvgl.pad_base, PAD_DEF);

		lv_style_set_pad_all(lvgl.pad_zero, 0);
		lv_style_set_pad_row(lvgl.pad_zero, 0);
		lv_style_set_pad_column(lvgl.pad_zero, 0);

		lv_style_set_pad_all(lvgl.pad_tiny, PAD_TINY);
		lv_style_set_pad_row(lvgl.pad_tiny, PAD_TINY);
		lv_style_set_pad_column(lvgl.pad_tiny, PAD_TINY);

		lv_style_set_radius(lvgl.circle, LV_RADIUS_CIRCLE);

		lv_style_set_radius(lvgl.no_radius, 0);

		lv_style_set_border_width(lvgl.no_border, 0);

		lv_style_set_rotary_sensitivity(lvgl.rotary_scroll, lv_display_get_dpi(NULL) / 4 * 256);

		lv_style_set_transform_width(lvgl.grow, LV_DPX_CALC(lv_display_get_dpi(NULL), 3));
		lv_style_set_transform_height(lvgl.grow, LV_DPX_CALC(lv_display_get_dpi(NULL), 3));

		lv_style_set_bg_color(lvgl.knob, m_colors.primary);
		lv_style_set_bg_opa(lvgl.knob, LV_OPA_COVER);
		lv_style_set_pad_all(lvgl.knob, LV_DPX_CALC(lv_display_get_dpi(NULL), 6));
		lv_style_set_radius(lvgl.knob, LV_RADIUS_CIRCLE);

		lv_style_set_anim_duration(lvgl.anim, 200);

		lv_style_set_anim_duration(lvgl.anim_fast, 120);

		lv_style_set_border_color(lvgl.input, m_colors.border);
		lv_style_set_border_width(lvgl.input, 1);
		lv_style_set_pad_hor(lvgl.input, 20);
		// lv_style_set_radius(lvgl.input, 2);
		lv_style_set_pad_ver(lvgl.input, 0);
		lv_style_set_text_align(lvgl.input, LV_TEXT_ALIGN_CENTER);

		// lv_style_set_transform_width(lvgl.draggable, LV_DPX(4));
		// lv_style_set_transform_height(lvgl.draggable, LV_DPX(4));
		// lv_style_set_transform_skew_x(lvgl.draggable, 2);
		// lv_style_set_transform_skew_y(lvgl.draggable, -2);

		static const lv_style_prop_t draggable_props[] = {LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT};
		lv_style_transition_dsc_init(&m_draggableTransition, draggable_props, lv_anim_path_ease_in_out, 200, 100, NULL);
		lv_style_set_transition(lvgl.draggable, &m_draggableTransition);

		static const lv_style_prop_t dragging_props[] = {
			LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_BORDER_WIDTH};
		lv_style_transition_dsc_init(
			&m_draggingTransition, dragging_props, lv_anim_path_ease_in_out, TRANSITION_TIME, 0, NULL);
		lv_style_set_transition(lvgl.dragging, &m_draggingTransition);

		lv_style_set_transform_width(lvgl.dragging, 20);
		lv_style_set_transform_height(lvgl.dragging, 20);
		lv_style_set_border_color(lvgl.dragging, lv_color_white());
		lv_style_set_border_opa(lvgl.dragging, LV_OPA_30);
		lv_style_set_border_width(lvgl.dragging, 20);

		static const lv_style_prop_t drag_complete_props[] = {LV_STYLE_OUTLINE_WIDTH};
		lv_style_transition_dsc_init(
			&m_dragCompleteTransition, drag_complete_props, lv_anim_path_ease_in_out, TRANSITION_TIME, 0, NULL);
		lv_style_set_transition(lvgl.dragging, &m_dragCompleteTransition);

		lv_style_set_outline_color(lvgl.drag_complete, lv_color_white());
		lv_style_set_outline_width(lvgl.drag_complete, 3);

#if LV_USE_ARC
		lv_style_set_arc_color(lvgl.arc_indic, m_colors.highlight);
		lv_style_set_arc_width(lvgl.arc_indic, LV_DPX_CALC(lv_display_get_dpi(NULL), 15));
		lv_style_set_arc_rounded(lvgl.arc_indic, true);

		lv_style_set_arc_color(lvgl.arc_indic_primary, m_colors.primary);
#endif

#if LV_USE_BAR
		lv_style_set_bg_color(lvgl.bar, m_colors.highlight);
		lv_style_set_bg_opa(lvgl.bar, LV_OPA_COVER);
		lv_style_set_radius(lvgl.bar, LV_RADIUS_CIRCLE);
		lv_style_set_radius(lvgl.bar_indic, LV_RADIUS_CIRCLE);
#endif

#if LV_USE_BUTTONMATRIX
		lv_style_set_bg_color(lvgl.btnm_btn, m_colors.bg_light);
		lv_style_set_bg_grad_dir(lvgl.btnm_btn, LV_GRAD_DIR_NONE);
		lv_style_merge(lvgl.btnm_btn, lvgl.border_muted);
		lv_style_set_shadow_width(lvgl.btnm_btn, 0);
		lv_style_set_radius(lvgl.btnm_btn, RADIUS_DEFAULT);
		lv_style_set_bg_color(lvgl.btnm_btn_checked, m_colors.primary);
		lv_style_set_bg_opa(lvgl.btnm_btn_checked, LV_OPA_COVER);
		lv_style_set_text_color(lvgl.btnm_btn_checked, m_colors.text);
#endif

#if LV_USE_DROPDOWN
		lv_style_set_pad_ver(lvgl.dropdown, PAD_DEF);
		lv_style_set_max_height(lvgl.dropdown_list, LV_DPI_DEF * 2);
#endif
#if LV_USE_CHECKBOX
		lv_style_set_pad_all(lvgl.cb_marker, LV_DPX_CALC(lv_display_get_dpi(NULL), 3));
		lv_style_set_border_width(lvgl.cb_marker, BORDER_WIDTH);
		lv_style_set_border_color(lvgl.cb_marker, m_colors.primary);
		lv_style_set_bg_color(lvgl.cb_marker, m_colors.bg);
		lv_style_set_bg_opa(lvgl.cb_marker, LV_OPA_COVER);
		lv_style_set_radius(lvgl.cb_marker, RADIUS_DEFAULT / 2);

		lv_style_set_bg_image_src(lvgl.cb_marker_checked, LV_SYMBOL_OK);
#endif

#if LV_USE_SWITCH
		lv_style_set_bg_color(lvgl.bg_switch, m_colors.bg_light);
		lv_style_set_bg_opa(lvgl.bg_switch, LV_OPA_COVER);
		lv_style_set_radius(lvgl.bg_switch, LV_RADIUS_CIRCLE);
		lv_style_set_pad_all(lvgl.switch_knob, -LV_DPX_CALC(lv_display_get_dpi(NULL), 4));
		lv_style_set_bg_color(lvgl.switch_knob, m_colors.text);
		lv_style_set_radius(lvgl.switch_knob, LV_RADIUS_CIRCLE);
#endif

#if LV_USE_LINE
		lv_style_set_line_width(lvgl.line, 1);
		lv_style_set_line_color(lvgl.line, m_colors.highlight);
#endif

#if LV_USE_CHART
		lv_style_set_border_width(lvgl.chart_bg, BORDER_WIDTH);
		lv_style_set_border_color(lvgl.chart_bg, m_colors.highlight);
		lv_style_set_border_opa(lvgl.chart_bg, LV_OPA_COVER);
		lv_style_set_border_post(lvgl.chart_bg, false);
		lv_style_set_pad_column(lvgl.chart_bg, LV_DPX_CALC(lv_display_get_dpi(NULL), 10));
		lv_style_set_line_color(lvgl.chart_bg, m_colors.highlight);

		lv_style_set_line_width(lvgl.chart_series, LV_DPX_CALC(lv_display_get_dpi(NULL), 3));
		lv_style_set_radius(lvgl.chart_series, LV_DPX_CALC(lv_display_get_dpi(NULL), 3));

		int32_t chart_size = LV_DPX_CALC(lv_display_get_dpi(NULL), 8);
		lv_style_set_size(lvgl.chart_series, chart_size, chart_size);
		lv_style_set_pad_column(lvgl.chart_series, LV_DPX_CALC(lv_display_get_dpi(NULL), 2));

		lv_style_set_radius(lvgl.chart_indic, LV_RADIUS_CIRCLE);
		lv_style_set_size(lvgl.chart_indic, chart_size, chart_size);
		lv_style_set_bg_color(lvgl.chart_indic, m_colors.primary);
		lv_style_set_bg_opa(lvgl.chart_indic, LV_OPA_COVER);
#endif

#if LV_USE_MENU
		lv_style_set_pad_all(lvgl.menu_bg, 0);
		lv_style_set_pad_gap(lvgl.menu_bg, 0);
		lv_style_set_radius(lvgl.menu_bg, 0);
		lv_style_set_clip_corner(lvgl.menu_bg, true);
		lv_style_set_border_side(lvgl.menu_bg, LV_BORDER_SIDE_NONE);

		lv_style_set_radius(lvgl.menu_section, RADIUS_DEFAULT);
		lv_style_set_clip_corner(lvgl.menu_section, true);
		lv_style_set_bg_opa(lvgl.menu_section, LV_OPA_COVER);
		lv_style_set_bg_color(lvgl.menu_section, m_colors.bg);
		lv_style_set_text_color(lvgl.menu_section, m_colors.text);

		lv_style_set_pad_hor(lvgl.menu_cont, PAD_SMALL);
		lv_style_set_pad_ver(lvgl.menu_cont, PAD_SMALL);
		lv_style_set_pad_gap(lvgl.menu_cont, PAD_SMALL);
		lv_style_set_border_width(lvgl.menu_cont, LV_DPX_CALC(lv_display_get_dpi(NULL), 1));
		lv_style_set_border_opa(lvgl.menu_cont, LV_OPA_10);
		lv_style_set_border_color(lvgl.menu_cont, m_colors.text);
		lv_style_set_border_side(lvgl.menu_cont, LV_BORDER_SIDE_NONE);

		lv_style_set_pad_all(lvgl.menu_sidebar_cont, 0);
		lv_style_set_pad_gap(lvgl.menu_sidebar_cont, 0);
		lv_style_set_border_width(lvgl.menu_sidebar_cont, LV_DPX_CALC(lv_display_get_dpi(NULL), 1));
		lv_style_set_border_opa(lvgl.menu_sidebar_cont, LV_OPA_10);
		lv_style_set_border_color(lvgl.menu_sidebar_cont, m_colors.text);
		lv_style_set_border_side(lvgl.menu_sidebar_cont, LV_BORDER_SIDE_RIGHT);

		lv_style_set_pad_all(lvgl.menu_main_cont, 0);
		lv_style_set_pad_gap(lvgl.menu_main_cont, 0);

		lv_style_set_pad_hor(lvgl.menu_header_cont, PAD_SMALL);
		lv_style_set_pad_ver(lvgl.menu_header_cont, PAD_TINY);
		lv_style_set_pad_gap(lvgl.menu_header_cont, PAD_SMALL);

		lv_style_set_pad_hor(lvgl.menu_header_btn, PAD_TINY);
		lv_style_set_pad_ver(lvgl.menu_header_btn, PAD_TINY);
		lv_style_set_shadow_opa(lvgl.menu_header_btn, LV_OPA_TRANSP);
		lv_style_set_bg_opa(lvgl.menu_header_btn, LV_OPA_TRANSP);
		lv_style_set_text_color(lvgl.menu_header_btn, m_colors.text);

		lv_style_set_pad_hor(lvgl.menu_page, 0);
		lv_style_set_pad_gap(lvgl.menu_page, 0);

		lv_style_set_bg_opa(lvgl.menu_pressed, LV_OPA_20);
		lv_style_set_bg_color(lvgl.menu_pressed, m_colors.bg_light);

		lv_style_set_bg_opa(lvgl.menu_separator, LV_OPA_TRANSP);
		lv_style_set_pad_ver(lvgl.menu_separator, PAD_TINY);
#endif

#if LV_USE_TABLE
		lv_style_set_radius(lvgl.table, 0);
		lv_style_set_bg_color(lvgl.table_cell, m_colors.bg);
		lv_style_set_bg_opa(lvgl.table_cell, LV_OPA_COVER);
		lv_style_set_text_color(lvgl.table_cell, m_colors.text);
		lv_style_set_border_width(lvgl.table_cell, LV_DPX_CALC(lv_display_get_dpi(NULL), 1));
		lv_style_set_border_color(lvgl.table_cell, m_colors.highlight);
		lv_style_set_border_side(lvgl.table_cell,
								 static_cast<lv_border_side_t>(LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM));
#endif

#if LV_USE_TEXTAREA
		lv_style_set_pad_all(lvgl.text_area, PAD_DEF);
		lv_style_set_border_color(lvgl.ta_cursor, m_colors.text);
		lv_style_set_border_width(lvgl.ta_cursor, LV_DPX_CALC(lv_display_get_dpi(NULL), 2));
		lv_style_set_pad_left(lvgl.ta_cursor, -LV_DPX_CALC(lv_display_get_dpi(NULL), 1));
		lv_style_set_border_side(lvgl.ta_cursor, LV_BORDER_SIDE_LEFT);
		lv_style_set_anim_duration(lvgl.ta_cursor, 400);

		lv_style_set_text_color(lvgl.ta_placeholder, m_colors.text_muted);
#endif

#if LV_USE_CALENDAR
		lv_style_set_pad_all(lvgl.calendar_btnm_bg, PAD_SMALL);
		lv_style_set_pad_gap(lvgl.calendar_btnm_bg, PAD_SMALL / 2);

		lv_style_set_border_width(lvgl.calendar_btnm_day, LV_DPX_CALC(lv_display_get_dpi(NULL), 1));
		lv_style_set_border_color(lvgl.calendar_btnm_day, m_colors.highlight);
		lv_style_set_bg_color(lvgl.calendar_btnm_day, m_colors.bg);
		lv_style_set_bg_opa(lvgl.calendar_btnm_day, LV_OPA_20);

		lv_style_set_pad_hor(lvgl.calendar_header, PAD_SMALL);
		lv_style_set_pad_top(lvgl.calendar_header, PAD_SMALL);
		lv_style_set_pad_bottom(lvgl.calendar_header, PAD_TINY);
		lv_style_set_pad_gap(lvgl.calendar_header, PAD_SMALL);
#endif

#if LV_USE_CANVAS
		lv_style_merge(lvgl.canvas, lvgl.bg);
		lv_style_merge(lvgl.canvas, lvgl.border);
#endif

#if LV_USE_MSGBOX
		// lv_style_set_bg_color(lvgl.msgbox_backdrop_bg, m_colors.bg);
		// lv_style_set_bg_opa(lvgl.msgbox_backdrop_bg, LV_OPA_50);
#endif
#if LV_USE_KEYBOARD
		lv_style_set_bg_color(lvgl.keyboard_button, m_colors.bg_light);
		lv_style_merge(lvgl.keyboard_button, lvgl.border_muted);
		lv_style_set_shadow_width(lvgl.keyboard_button, 0);
		lv_style_set_radius(lvgl.keyboard_button, RADIUS_DEFAULT);
		lv_style_set_bg_color(lvgl.keyboard_button_checked_bg, m_colors.primary);
		lv_style_set_bg_opa(lvgl.keyboard_button_checked_bg, LV_OPA_COVER);
		lv_style_set_text_color(lvgl.keyboard_button_checked_bg, m_colors.text);
#endif

#if LV_USE_TABVIEW
		lv_style_set_border_color(lvgl.tab_btn, m_colors.primary);
		lv_style_set_border_width(lvgl.tab_btn, BORDER_WIDTH * 2);
		lv_style_set_border_side(lvgl.tab_btn, LV_BORDER_SIDE_BOTTOM);
		lv_style_set_pad_top(lvgl.tab_btn, BORDER_WIDTH * 2);

		lv_style_set_outline_pad(lvgl.tab_bg_focus, -BORDER_WIDTH);
#endif

#if LV_USE_LIST
		lv_style_set_pad_hor(lvgl.list_bg, PAD_DEF);
		lv_style_set_pad_ver(lvgl.list_bg, 0);
		lv_style_set_pad_gap(lvgl.list_bg, 0);
		lv_style_set_clip_corner(lvgl.list_bg, true);

		lv_style_set_border_width(lvgl.list_btn, 1);
		lv_style_set_border_color(lvgl.list_btn, m_colors.border);
		lv_style_set_border_side(lvgl.list_btn, LV_BORDER_SIDE_BOTTOM);
		lv_style_set_pad_all(lvgl.list_btn, PAD_SMALL);
		lv_style_set_pad_column(lvgl.list_btn, PAD_SMALL);

		lv_style_set_transform_width(lvgl.list_item_grow, PAD_DEF);
#endif

#if LV_USE_LED
		lv_style_set_bg_opa(lvgl.led, LV_OPA_COVER);
		lv_style_set_bg_color(lvgl.led, lv_color_white());
		lv_style_set_bg_grad_color(lvgl.led, m_colors.bg_dark);
		lv_style_set_radius(lvgl.led, LV_RADIUS_CIRCLE);
		lv_style_set_shadow_width(lvgl.led, LV_DPX_CALC(lv_display_get_dpi(NULL), 15));
		lv_style_set_shadow_color(lvgl.led, lv_color_white());
		lv_style_set_shadow_spread(lvgl.led, LV_DPX_CALC(lv_display_get_dpi(NULL), 5));
#endif

#if LV_USE_SCALE
		lv_style_set_line_color(lvgl.scale, m_colors.text);
		lv_style_set_line_width(lvgl.scale, LV_DPX(2));
		lv_style_set_arc_color(lvgl.scale, m_colors.text);
		lv_style_set_arc_width(lvgl.scale, LV_DPX(2));
		lv_style_set_length(lvgl.scale, LV_DPX(6));
#endif

#if LV_USE_SLIDER
		lv_style_set_height(lvgl.slider, 20);
		lv_style_set_bg_color(lvgl.slider, m_colors.highlight);
		lv_style_set_bg_opa(lvgl.slider, LV_OPA_COVER);
		lv_style_set_radius(lvgl.slider, RADIUS_DEFAULT);
		lv_style_set_radius(lvgl.slider_indic, RADIUS_DEFAULT);
		lv_style_set_radius(lvgl.slider_knob, RADIUS_DEFAULT);
		lv_style_set_border_color(lvgl.slider_knob, m_colors.primary_muted);
		lv_style_set_border_width(lvgl.slider_knob, BORDER_WIDTH);
#endif

		lv_style_set_recolor(components.console_btn_anim, m_colors.secondary);

		lv_style_set_bg_color(components.estop, lv_palette_main(LV_PALETTE_RED));
		// lv_style_set_bg_grad_color(components.estop, lv_palette_darken(LV_PALETTE_RED, 4));
		lv_style_set_text_color(components.estop, lv_color_white());
		lv_style_set_radius(components.estop, LV_RADIUS_CIRCLE);

		lv_style_copy(components.folder, lvgl.btn);

		lv_style_merge(components.file, lvgl.btn);
		lv_style_merge(components.file, lvgl.bg_light);
		lv_style_merge(components.file, lvgl.border);

		lv_style_set_bg_color(components.unhomed, lv_color_hex(0xfb9514));
		lv_style_set_border_width(components.unhomed, 0);

		lv_style_set_bg_opa(components.temperature_bar, LV_OPA_COVER);
		lv_style_set_bg_color(components.temperature_bar, lv_palette_main(LV_PALETTE_BLUE));
		lv_style_set_bg_grad_color(components.temperature_bar, lv_palette_main(LV_PALETTE_RED));
		lv_style_set_bg_grad_dir(components.temperature_bar, LV_GRAD_DIR_HOR);

		lv_style_set_bg_color(components.temperature_bar_indic, m_colors.border);
		lv_style_set_bg_opa(components.temperature_bar_indic, LV_OPA_COVER);

		lv_style_copy(components.tool_list_tool, lvgl.card);
		// lv_style_set_bg_color(components.tool_list_tool, m_colors.bg_light);
		// lv_style_set_bg_opa(components.tool_list_tool, LV_OPA_COVER);
		// lv_style_set_radius(components.tool_list_tool, RADIUS_DEFAULT);

		// lv_style_copy(components.tool_list_heater, components.tool_list_tool);
		lv_style_set_pad_all(components.tool_list_heater, 0);

		lv_style_set_outline_color(components.tool_selected, m_colors.primary);
		lv_style_set_outline_opa(components.tool_selected, LV_OPA_COVER);
		lv_style_set_outline_width(components.tool_selected, 3);
		lv_style_set_shadow_color(components.tool_selected, lv_color_darken(m_colors.primary, 10));
		lv_style_set_shadow_opa(components.tool_selected, LV_OPA_50);
		lv_style_set_shadow_width(components.tool_selected, 0);
		lv_style_set_shadow_offset_x(components.tool_selected, -SHADOW_OFFSET);
		lv_style_set_shadow_offset_y(components.tool_selected, SHADOW_OFFSET);

		lv_style_merge(components.list_title, lvgl.text_emphasis);

		lv_style_set_width(components.slider_buttons, 40);
		lv_style_set_height(components.slider_buttons, 40);

		/* Graph */

		lv_style_set_min_width(components.graph_legend, 150);
		// lv_style_set_max_width(components.graph_legend, 200);

		/* Bar */

		lv_style_set_bg_color(components.bar_label_bg, m_colors.bg_light);
		lv_style_set_bg_opa(components.bar_label_bg, LV_OPA_50);
		lv_style_set_radius(components.bar_label_bg, RADIUS_DEFAULT);
		lv_style_set_pad_hor(components.bar_label_bg, PAD_DEF);

		lv_style_copy(components.object_cancel_base, lvgl.card);
		lv_style_set_bg_color(components.object_cancel_base, m_colors.bg_light);
		lv_style_set_bg_opa(components.object_cancel_base, LV_OPA_50);

		lv_style_set_bg_color(components.object_cancel_cancelled, m_colors.error);
		lv_style_set_bg_opa(components.object_cancel_cancelled, LV_OPA_COVER);

		lv_style_set_bg_color(components.object_cancel_current, m_colors.primary);
		lv_style_set_bg_opa(components.object_cancel_current, LV_OPA_COVER);

#if DEBUG_BORDERS
		lv_style_set_outline_color(components.debug_borders, m_colors.text);
		lv_style_set_outline_width(components.debug_borders, 1);
		lv_style_set_outline_opa(components.debug_borders, LV_OPA_100);
#endif
	}
} // namespace UI::Themes

/*
 * ThreeDTheme.cpp
 *
 *  Created on: 2026-04-05
 *      Author: Andy Everitt
 *
 *  Skeuomorphic 3D theme using vertical gradients and directional shadows
 *  to give buttons, cards, and controls a raised, bevelled appearance.
 *  Light-to-dark gradients simulate top-down lighting; offset shadows
 *  provide depth beneath every interactive element.
 */

#include "CustomTheme.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI::Themes
{
	static ColorCtx s_colors = {.primaryHue = 220,
								.secondaryHue = 160,
								.chroma = 0.06f,
								.darkMode = false,
								.customizer = [](ThemeColors& colors, [[maybe_unused]] const ColorCtx& ctx)
								{
									/* Slightly lighten backgrounds so gradients have room to
									   darken towards the bottom without looking muddy. */
									colors.bg_light.setL(std::min(colors.bg_light.getL() + 0.04f, 1.0f));
								}};

	static FontConfigSet s_fontConfigs = {
		.header = {.size = 18, .style = LV_FREETYPE_FONT_STYLE_BOLD | LV_FREETYPE_FONT_STYLE_WEIGHT(700)},
		.normal = {.size = 14, .style = LV_FREETYPE_FONT_STYLE_NORMAL | LV_FREETYPE_FONT_STYLE_WEIGHT(450)},
		.emphasis = {.size = 14, .style = LV_FREETYPE_FONT_STYLE_BOLD | LV_FREETYPE_FONT_STYLE_WEIGHT(600)},
		.subdued = {.size = 12, .style = LV_FREETYPE_FONT_STYLE_NORMAL | LV_FREETYPE_FONT_STYLE_WEIGHT(350)},
	};

	static CustomTheme s_theme("3d",
							   s_colors,
							   s_fontConfigs,
							   [](Theme* theme)
							   {
								   auto t = static_cast<DefaultTheme*>(theme);
								   auto& lvgl = t->getLvglStyles();
								   auto& components = t->getComponentStyles();
								   auto& colors = t->getColors();

								   /* --- Helper: derive a darker shade for gradient bottoms --- */
								   auto darken = [](const Color& c, float amount) -> lv_color_t
								   {
									   Color d = c;
									   d.setL(std::max(c.getL() - amount, 0.0f));
									   return static_cast<lv_color_t>(d);
								   };

								   /* --- Helper: derive a lighter shade for highlights --- */
								   auto lighten = [](const Color& c, float amount) -> lv_color_t
								   {
									   Color l = c;
									   l.setL(std::min(c.getL() + amount, 1.0f));
									   return static_cast<lv_color_t>(l);
								   };

								   /* ========= Cards ========= */
								   /* Moderate rounding for a polished look */
								   lv_style_set_radius(lvgl.card, 8);

								   /* Top-to-bottom gradient: light top → slightly darker bottom */
								   lv_style_set_bg_color(lvgl.card, lighten(colors.bg, 0.04f));
								   lv_style_set_bg_grad_color(lvgl.card, darken(colors.bg, 0.06f));
								   lv_style_set_bg_grad_dir(lvgl.card, LV_GRAD_DIR_VER);

								   /* Subtle border with a highlight tint on top */
								   lv_style_set_border_width(lvgl.card, 1);
								   lv_style_set_border_color(lvgl.card, darken(colors.border, 0.05f));
								   lv_style_set_border_opa(lvgl.card, LV_OPA_60);
								   lv_style_set_border_side(lvgl.card, LV_BORDER_SIDE_FULL);

								   /* Drop shadow for raised appearance */
								   lv_style_set_shadow_width(lvgl.card, 10);
								   lv_style_set_shadow_spread(lvgl.card, 1);
								   lv_style_set_shadow_offset_x(lvgl.card, 0);
								   lv_style_set_shadow_offset_y(lvgl.card, 4);
								   lv_style_set_shadow_color(lvgl.card, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.card, LV_OPA_20);

								   /* Comfortable padding */
								   lv_style_set_pad_all(lvgl.card, 8);
								   lv_style_set_pad_row(lvgl.card, 4);
								   lv_style_set_pad_column(lvgl.card, 4);

								   /* ========= Buttons ========= */
								   lv_style_set_radius(lvgl.btn, 6);

								   /* Raised button: light top → darker bottom */
								   lv_style_set_bg_color(lvgl.btn, lighten(colors.primary, 0.08f));
								   lv_style_set_bg_grad_color(lvgl.btn, darken(colors.primary, 0.10f));
								   lv_style_set_bg_grad_dir(lvgl.btn, LV_GRAD_DIR_VER);

								   /* Thin highlight border */
								   lv_style_set_border_width(lvgl.btn, 1);
								   lv_style_set_border_color(lvgl.btn, lighten(colors.primary, 0.15f));
								   lv_style_set_border_opa(lvgl.btn, LV_OPA_40);
								   lv_style_set_border_side(lvgl.btn, LV_BORDER_SIDE_TOP);

								   /* Button drop shadow */
								   lv_style_set_shadow_width(lvgl.btn, 6);
								   lv_style_set_shadow_spread(lvgl.btn, 0);
								   lv_style_set_shadow_offset_x(lvgl.btn, 0);
								   lv_style_set_shadow_offset_y(lvgl.btn, 3);
								   lv_style_set_shadow_color(lvgl.btn, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.btn, LV_OPA_30);

								   /* Action button: same 3D treatment with secondary colour */
								   lv_style_set_bg_color(lvgl.actionBtn, lighten(colors.secondary, 0.08f));
								   lv_style_set_bg_grad_color(lvgl.actionBtn, darken(colors.secondary, 0.10f));
								   lv_style_set_bg_grad_dir(lvgl.actionBtn, LV_GRAD_DIR_VER);
								   lv_style_set_border_width(lvgl.actionBtn, 1);
								   lv_style_set_border_color(lvgl.actionBtn, lighten(colors.secondary, 0.15f));
								   lv_style_set_border_opa(lvgl.actionBtn, LV_OPA_40);
								   lv_style_set_border_side(lvgl.actionBtn, LV_BORDER_SIDE_TOP);

								   /* Checked button: inverted gradient (pressed-in look) */
								   lv_style_set_bg_color(lvgl.btn_checked, darken(colors.primary, 0.06f));
								   lv_style_set_bg_grad_color(lvgl.btn_checked, lighten(colors.primary, 0.04f));
								   lv_style_set_bg_grad_dir(lvgl.btn_checked, LV_GRAD_DIR_VER);
								   lv_style_set_shadow_width(lvgl.btn_checked, 2);
								   lv_style_set_shadow_offset_y(lvgl.btn_checked, 1);
								   lv_style_set_shadow_opa(lvgl.btn_checked, LV_OPA_20);

								   /* Pressed: darken + reduce shadow for pushed-in feel */
								   lv_style_set_recolor(lvgl.pressed, lv_color_black());
								   lv_style_set_recolor_opa(lvgl.pressed, 25);

								   /* ========= Knob ========= */
								   lv_style_set_radius(lvgl.knob, LV_RADIUS_CIRCLE);
								   lv_style_set_shadow_width(lvgl.knob, 6);
								   lv_style_set_shadow_offset_y(lvgl.knob, 2);
								   lv_style_set_shadow_color(lvgl.knob, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.knob, LV_OPA_30);

								   /* ========= Scrollbar ========= */
								   lv_style_set_radius(lvgl.scrollbar, 4);
								   lv_style_set_width(lvgl.scrollbar, 6);
								   lv_style_set_bg_opa(lvgl.scrollbar, LV_OPA_30);

#if LV_USE_BUTTONMATRIX
								   /* Button matrix: raised individual cells */
								   lv_style_set_radius(lvgl.btnm_btn, 5);
								   lv_style_set_bg_color(lvgl.btnm_btn, lighten(colors.bg, 0.03f));
								   lv_style_set_bg_grad_color(lvgl.btnm_btn, darken(colors.bg, 0.08f));
								   lv_style_set_bg_grad_dir(lvgl.btnm_btn, LV_GRAD_DIR_VER);
								   lv_style_set_border_width(lvgl.btnm_btn, 1);
								   lv_style_set_border_color(lvgl.btnm_btn, lighten(colors.bg, 0.06f));
								   lv_style_set_border_opa(lvgl.btnm_btn, LV_OPA_50);
								   lv_style_set_border_side(lvgl.btnm_btn, LV_BORDER_SIDE_TOP);
								   lv_style_set_shadow_width(lvgl.btnm_btn, 4);
								   lv_style_set_shadow_offset_y(lvgl.btnm_btn, 2);
								   lv_style_set_shadow_color(lvgl.btnm_btn, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.btnm_btn, LV_OPA_20);

								   /* Checked buttonmatrix item: 3D in primary colour */
								   lv_style_set_bg_color(lvgl.btnm_btn_checked, lighten(colors.primary, 0.06f));
								   lv_style_set_bg_grad_color(lvgl.btnm_btn_checked, darken(colors.primary, 0.08f));
								   lv_style_set_bg_grad_dir(lvgl.btnm_btn_checked, LV_GRAD_DIR_VER);
#endif

#if LV_USE_BAR
								   lv_style_set_radius(lvgl.bar, 6);
								   lv_style_set_radius(lvgl.bar_indic, 6);
#endif

#if LV_USE_SLIDER
								   /* Slider track: inset groove */
								   lv_style_set_radius(lvgl.slider, 6);
								   lv_style_set_height(lvgl.slider, 14);
								   lv_style_set_bg_color(lvgl.slider, darken(colors.highlight, 0.06f));
								   lv_style_set_bg_grad_color(lvgl.slider, lighten(colors.highlight, 0.03f));
								   lv_style_set_bg_grad_dir(lvgl.slider, LV_GRAD_DIR_VER);
								   lv_style_set_shadow_width(lvgl.slider, 4);
								   lv_style_set_shadow_offset_y(lvgl.slider, -1);
								   lv_style_set_shadow_color(lvgl.slider, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.slider, LV_OPA_10);

								   /* Slider indicator: raised fill */
								   lv_style_set_radius(lvgl.slider_indic, 6);
								   lv_style_set_bg_grad_color(lvgl.slider_indic, darken(colors.primary, 0.08f));
								   lv_style_set_bg_grad_dir(lvgl.slider_indic, LV_GRAD_DIR_VER);

								   /* Slider knob: raised with gradient and shadow */
								   lv_style_set_radius(lvgl.slider_knob, LV_RADIUS_CIRCLE);
								   lv_style_set_bg_color(lvgl.slider_knob, lighten(colors.primary, 0.12f));
								   lv_style_set_bg_grad_color(lvgl.slider_knob, darken(colors.primary, 0.06f));
								   lv_style_set_bg_grad_dir(lvgl.slider_knob, LV_GRAD_DIR_VER);
								   lv_style_set_shadow_width(lvgl.slider_knob, 8);
								   lv_style_set_shadow_offset_y(lvgl.slider_knob, 3);
								   lv_style_set_shadow_color(lvgl.slider_knob, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.slider_knob, LV_OPA_30);
								   lv_style_set_border_width(lvgl.slider_knob, 1);
								   lv_style_set_border_color(lvgl.slider_knob, lighten(colors.primary, 0.15f));
								   lv_style_set_border_opa(lvgl.slider_knob, LV_OPA_40);
								   lv_style_set_border_side(lvgl.slider_knob, LV_BORDER_SIDE_TOP);
#endif

#if LV_USE_CHECKBOX
								   /* Checkbox marker: inset well with gradient */
								   lv_style_set_radius(lvgl.cb_marker, 4);
								   lv_style_set_bg_color(lvgl.cb_marker, darken(colors.bg, 0.04f));
								   lv_style_set_bg_grad_color(lvgl.cb_marker, lighten(colors.bg, 0.02f));
								   lv_style_set_bg_grad_dir(lvgl.cb_marker, LV_GRAD_DIR_VER);
								   lv_style_set_border_width(lvgl.cb_marker, 1);
								   lv_style_set_border_color(lvgl.cb_marker, darken(colors.border, 0.08f));
								   lv_style_set_shadow_width(lvgl.cb_marker, 4);
								   lv_style_set_shadow_offset_y(lvgl.cb_marker, -1);
								   lv_style_set_shadow_color(lvgl.cb_marker, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.cb_marker, LV_OPA_10);
#endif

#if LV_USE_SWITCH
								   /* Switch track: inset groove */
								   lv_style_set_radius(lvgl.bg_switch, LV_RADIUS_CIRCLE);
								   lv_style_set_bg_grad_color(lvgl.bg_switch, darken(colors.bg_light, 0.06f));
								   lv_style_set_bg_grad_dir(lvgl.bg_switch, LV_GRAD_DIR_VER);
								   lv_style_set_shadow_width(lvgl.bg_switch, 3);
								   lv_style_set_shadow_offset_y(lvgl.bg_switch, -1);
								   lv_style_set_shadow_color(lvgl.bg_switch, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.bg_switch, LV_OPA_10);

								   /* Switch knob: raised pill */
								   lv_style_set_radius(lvgl.switch_knob, LV_RADIUS_CIRCLE);
								   lv_style_set_shadow_width(lvgl.switch_knob, 5);
								   lv_style_set_shadow_offset_y(lvgl.switch_knob, 2);
								   lv_style_set_shadow_color(lvgl.switch_knob, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.switch_knob, LV_OPA_20);
#endif

#if LV_USE_KEYBOARD
								   /* Keyboard keys: raised 3D buttons with gradient */
								   lv_style_set_radius(lvgl.keyboard_button, 5);
								   lv_style_set_bg_color(lvgl.keyboard_button, lighten(colors.bg_light, 0.03f));
								   lv_style_set_bg_grad_color(lvgl.keyboard_button, darken(colors.bg_light, 0.06f));
								   lv_style_set_bg_grad_dir(lvgl.keyboard_button, LV_GRAD_DIR_VER);
								   lv_style_set_shadow_width(lvgl.keyboard_button, 4);
								   lv_style_set_shadow_offset_y(lvgl.keyboard_button, 2);
								   lv_style_set_shadow_color(lvgl.keyboard_button, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.keyboard_button, LV_OPA_20);
								   lv_style_set_border_width(lvgl.keyboard_button, 1);
								   lv_style_set_border_color(lvgl.keyboard_button, lighten(colors.bg_light, 0.06f));
								   lv_style_set_border_opa(lvgl.keyboard_button, LV_OPA_50);
								   lv_style_set_border_side(lvgl.keyboard_button, LV_BORDER_SIDE_TOP);

								   /* Checked keyboard key: 3D in primary colour */
								   lv_style_set_bg_color(lvgl.keyboard_button_checked_bg,
														 lighten(colors.primary, 0.06f));
								   lv_style_set_bg_grad_color(lvgl.keyboard_button_checked_bg,
															  darken(colors.primary, 0.08f));
								   lv_style_set_bg_grad_dir(lvgl.keyboard_button_checked_bg, LV_GRAD_DIR_VER);
#endif

#if LV_USE_TABVIEW
								   lv_style_set_border_width(lvgl.tab_btn, 0);
								   lv_style_set_shadow_width(lvgl.tab_btn, 3);
								   lv_style_set_shadow_offset_y(lvgl.tab_btn, 1);
								   lv_style_set_shadow_color(lvgl.tab_btn, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.tab_btn, LV_OPA_10);
#endif

#if LV_USE_LIST
								   lv_style_set_pad_all(lvgl.list_btn, 6);
								   lv_style_set_border_width(lvgl.list_btn, 0);
								   lv_style_set_shadow_width(lvgl.list_btn, 3);
								   lv_style_set_shadow_offset_y(lvgl.list_btn, 1);
								   lv_style_set_shadow_opa(lvgl.list_btn, LV_OPA_10);
								   lv_style_set_shadow_color(lvgl.list_btn, lv_color_black());
#endif

								   /* ========= Component adjustments ========= */
								   lv_style_set_radius(components.bar_label_bg, 6);

								   /* E-stop: 3D red gradient instead of inheriting button gradient */
								   lv_style_set_bg_color(components.estop, lv_palette_lighten(LV_PALETTE_RED, 1));
								   lv_style_set_bg_grad_color(components.estop, lv_palette_darken(LV_PALETTE_RED, 3));
								   lv_style_set_bg_grad_dir(components.estop, LV_GRAD_DIR_VER);
								   lv_style_set_shadow_width(components.estop, 8);
								   lv_style_set_shadow_offset_y(components.estop, 3);
								   lv_style_set_shadow_color(components.estop, lv_palette_darken(LV_PALETTE_RED, 4));
								   lv_style_set_shadow_opa(components.estop, LV_OPA_40);

								   lv_style_copy(components.folder, lvgl.btn);

								   lv_style_merge(components.file, lvgl.btn);
								   lv_style_merge(components.file, lvgl.bg_light);
								   lv_style_set_pad_hor(components.file, 12);
								   lv_style_set_pad_ver(components.file, 10);
							   });
} // namespace UI::Themes

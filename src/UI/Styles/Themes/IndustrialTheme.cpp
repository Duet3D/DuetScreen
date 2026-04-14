/*
 * IndustrialTheme.cpp
 *
 *  Created on: 2026-02-12
 *      Author: Andy Everitt
 *
 *  Angular, mechanical look: sharp corners, thick borders, dark background
 *  with orange accents. No rounded edges, heavy separators.
 */

#include "CustomTheme.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"

namespace UI::Themes
{
	static ColorCtx s_colors = {.primaryHue = 245, .secondaryHue = 50, .chroma = 0.02f, .darkMode = false};
	static FontConfigSet s_fontConfigs = {
		.header = {.size = 22, .style = LV_FREETYPE_FONT_STYLE_BOLD | LV_FREETYPE_FONT_STYLE_WEIGHT(800)},
		.normal = {.size = 18, .style = LV_FREETYPE_FONT_STYLE_NORMAL | LV_FREETYPE_FONT_STYLE_WEIGHT(500)},
		.emphasis = {.size = 20, .style = LV_FREETYPE_FONT_STYLE_BOLD | LV_FREETYPE_FONT_STYLE_WEIGHT(700)},
		.subdued = {.size = 16, .style = LV_FREETYPE_FONT_STYLE_NORMAL | LV_FREETYPE_FONT_STYLE_WEIGHT(300)},
	};

	static CustomTheme s_theme("industrial",
							   s_colors,
							   s_fontConfigs,
							   [](Theme* theme)
							   {
								   auto t = static_cast<DefaultTheme*>(theme);
								   auto& lvgl = t->getLvglStyles();
								   auto& components = t->getComponentStyles();
								   auto& colors = t->getColors();

								   /* Sharp corners everywhere */
								   lv_style_set_radius(lvgl.card, 0);
								   lv_style_set_radius(lvgl.btn, 0);
								   lv_style_set_radius(lvgl.knob, 2);

								   lv_style_set_bg_color(lvgl.btn_checked, colors.primary);
								   lv_style_set_text_color(lvgl.btn_checked, colors.text);

								   /* Thick, prominent borders */
								   lv_style_set_border_width(lvgl.card, 3);
								   lv_style_set_border_side(lvgl.card, LV_BORDER_SIDE_FULL);

								   /* Buttons: flat, angular, bordered */
								   lv_style_set_bg_color(lvgl.btn, colors.bg_light);
								   lv_style_set_border_width(lvgl.btn, 2);
								   lv_style_set_border_color(lvgl.btn, colors.primary);
								   lv_style_set_border_side(lvgl.btn, LV_BORDER_SIDE_FULL);
								   lv_style_set_text_color(lvgl.btn, colors.primary);

								   lv_style_set_bg_color(lvgl.actionBtn, colors.primary);
								   lv_style_set_text_color(lvgl.actionBtn, colors.text);

								   /* Heavier padding for an industrial panel feel */
								   lv_style_set_pad_all(lvgl.card, 8);
								   lv_style_set_pad_row(lvgl.card, 4);
								   lv_style_set_pad_column(lvgl.card, 4);

								   /* Shadow: hard offset, no blur for a stamped look */
								   lv_style_set_shadow_width(lvgl.card, 0);
								   lv_style_set_shadow_offset_x(lvgl.card, 3);
								   lv_style_set_shadow_offset_y(lvgl.card, 3);
								   lv_style_set_shadow_color(lvgl.card, lv_color_black());
								   lv_style_set_shadow_opa(lvgl.card, LV_OPA_60);

								   /* Scrollbar: thin rectangular */
								   lv_style_set_radius(lvgl.scrollbar, 0);
								   lv_style_set_width(lvgl.scrollbar, 4);

#if LV_USE_BUTTONMATRIX
								   lv_style_set_radius(lvgl.btnm_btn, 0);
								   lv_style_set_border_width(lvgl.btnm_btn, 2);
								   lv_style_set_border_color(lvgl.btnm_btn, colors.border);
#endif

#if LV_USE_BAR
								   lv_style_set_radius(lvgl.bar, 0);
								   lv_style_set_radius(lvgl.bar_indic, 0);
#endif
#if LV_USE_ARC
								   lv_style_set_arc_rounded(lvgl.arc_indic, false);
#endif

#if LV_USE_SLIDER
								   lv_style_set_radius(lvgl.slider, 0);
								   lv_style_set_radius(lvgl.slider_indic, 0);
								   lv_style_set_radius(lvgl.slider_knob, 2);
								   lv_style_set_border_width(lvgl.slider_knob, 2);
#endif

#if LV_USE_CHECKBOX
								   lv_style_set_radius(lvgl.cb_marker, 0);
#endif

#if LV_USE_SWITCH
								   lv_style_set_radius(lvgl.bg_switch, 2);
								   lv_style_set_radius(lvgl.switch_knob, 1);
#endif

#if LV_USE_KEYBOARD
								   lv_style_set_radius(lvgl.keyboard_button, 0);
#endif

#if LV_USE_TABVIEW
								   lv_style_set_border_width(lvgl.tab_btn, 3);
								   lv_style_set_border_side(lvgl.tab_btn, LV_BORDER_SIDE_BOTTOM);
#endif

								   lv_style_set_radius(components.bar_label_bg, 0);
								   lv_style_set_radius(components.estop, 4);

								   lv_style_copy(components.folder, lvgl.btn);
								   lv_style_merge(components.file, lvgl.btn);
								   lv_style_merge(components.file, lvgl.bg_light);
							   });
} // namespace UI::Themes

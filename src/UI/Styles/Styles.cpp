/*
 * Styles.cpp
 *
 *  Created on: 2025-01-21
 *      Author: Andy Everitt
 */

#include "Styles.h"
#include "Debug.h"
#include "UI/Core/View.h"
#include "lvgl/src/lvgl_private.h"
#include "utils/StorageHelper.h"
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <utility>

namespace UI::Themes
{
	static lv_theme_t s_theme;
	// Use function-local statics to avoid cross-TU static initialization order issues
	static std::vector<Style*>& uninitializedStyles()
	{
		static std::vector<Style*> v;
		return v;
	}

#define STATIC_ON_FIRST_USE(type, name)                                                                                \
	static type& name()                                                                                                \
	{                                                                                                                  \
		static type s_##name;                                                                                          \
		return s_##name;                                                                                               \
	}

	static std::unique_ptr<LvglStyles> s_lvglStyles;
	static std::unique_ptr<ComponentStyles> s_componentStyles;

	static Fonts s_fonts;

	Style::Style()
		: name(nullptr)
		, initFunc(nullptr)
	{
		ZoneScoped;
		lv_style_init(&style);
	}

	Style::Style(const char* name)
		: name(name)
		, initFunc(nullptr)
	{
		ZoneScoped;
		lv_style_init(&style);
	}

	Style::Style(const char* name, std::function<void(lv_style_t*)> initFunc)
		: Style(name)
	{
		ZoneScoped;
		if (!lv_is_initialized())
		{
			LOG_DBG("LVGL not initialized, deferring style initialization");
			this->initFunc = initFunc;
			uninitializedStyles().push_back(this);
		}
		else
		{
			initFunc(&style);
		}
	}

	Style::Style(const Style& other)
		: name(other.name)
	{
		ZoneScoped;
		UI_LOCK();
		lv_style_copy(&style, &other.style);
	}

	Style& Style::operator=(const Style& other)
	{
		ZoneScoped;
		UI_LOCK();
		lv_style_copy(&style, &other.style);
		return *this;
	}

	Style::Style(Style&& other)
	{
		ZoneScoped;
		UI_LOCK();
		style = other.style;
		other.m_moved = true;
	}

	Style& Style::operator=(Style&& other)
	{
		ZoneScoped;
		UI_LOCK();
		style = other.style;
		other.m_moved = true;
		return *this;
	}

	Style::~Style()
	{
		ZoneScoped;
		if (!m_moved)
			lv_style_reset(&style);
	}

	void Style::init()
	{
		ZoneScoped;
		if (initFunc)
		{
			initFunc(&style);
			initFunc = nullptr;
		}
	}

	LvglStyles::LvglStyles() {}

	const LvglStyles& getLvglStyles()
	{
		ZoneScoped;
		if (!s_lvglStyles)
		{
			s_lvglStyles = std::make_unique<LvglStyles>();
		}
		return *s_lvglStyles;
	}

	const ComponentStyles& getComponentStyles()
	{
		ZoneScoped;
		if (!s_componentStyles)
		{
			s_componentStyles = std::make_unique<ComponentStyles>();
		}
		return *s_componentStyles;
	}

	/* Every style by name, for forEachStyle(). The name is the member name; XML and themes use it. */
	static constexpr std::pair<const char*, Style LvglStyles::*> s_lvglStyleNames[] = {
		{"base", &LvglStyles::base},
		{"bg_dark", &LvglStyles::bg_dark},
		{"bg", &LvglStyles::bg},
		{"bg_light", &LvglStyles::bg_light},
		{"bg_color_primary", &LvglStyles::bg_color_primary},
		{"bg_color_primary_muted", &LvglStyles::bg_color_primary_muted},
		{"bg_color_secondary", &LvglStyles::bg_color_secondary},
		{"bg_color_secondary_muted", &LvglStyles::bg_color_secondary_muted},
		{"bg_color_success", &LvglStyles::bg_color_success},
		{"bg_color_warning", &LvglStyles::bg_color_warning},
		{"bg_color_error", &LvglStyles::bg_color_error},
		{"bg_color_header", &LvglStyles::bg_color_header},
		{"bg_color_list_item", &LvglStyles::bg_color_list_item},
		{"bg_modal", &LvglStyles::bg_modal},
		{"text", &LvglStyles::text},
		{"text_muted", &LvglStyles::text_muted},
		{"text_emphasis", &LvglStyles::text_emphasis},
		{"text_header", &LvglStyles::text_header},
		{"text_warning", &LvglStyles::text_warning},
		{"border", &LvglStyles::border},
		{"border_muted", &LvglStyles::border_muted},
		{"border_highlight", &LvglStyles::border_highlight},
		{"border_color_primary", &LvglStyles::border_color_primary},
		{"border_color_secondary", &LvglStyles::border_color_secondary},
		{"border_color_card", &LvglStyles::border_color_card},
		{"screen", &LvglStyles::screen},
		{"scrollbar", &LvglStyles::scrollbar},
		{"scrollbar_scrolled", &LvglStyles::scrollbar_scrolled},
		{"card", &LvglStyles::card},
		{"tab_card", &LvglStyles::tab_card},
		{"modal", &LvglStyles::modal},
		{"btn", &LvglStyles::btn},
		{"btn_checked", &LvglStyles::btn_checked},
		{"icon", &LvglStyles::icon},
		{"icon_recolor", &LvglStyles::icon_recolor},
		{"pressed", &LvglStyles::pressed},
		{"disabled", &LvglStyles::disabled},
		{"pad_base", &LvglStyles::pad_base},
		{"pad_zero", &LvglStyles::pad_zero},
		{"pad_tiny", &LvglStyles::pad_tiny},
		{"pad_small", &LvglStyles::pad_small},
		{"pad_normal", &LvglStyles::pad_normal},
		{"pad_large", &LvglStyles::pad_large},
		{"pad_gap", &LvglStyles::pad_gap},
		{"line_space_large", &LvglStyles::line_space_large},
		{"text_align_center", &LvglStyles::text_align_center},
		{"outline_primary", &LvglStyles::outline_primary},
		{"outline_secondary", &LvglStyles::outline_secondary},
		{"circle", &LvglStyles::circle},
		{"no_radius", &LvglStyles::no_radius},
		{"no_border", &LvglStyles::no_border},
		{"clip_corner", &LvglStyles::clip_corner},
		{"rotary_scroll", &LvglStyles::rotary_scroll},
		{"grow", &LvglStyles::grow},
		{"transition_delayed", &LvglStyles::transition_delayed},
		{"transition_normal", &LvglStyles::transition_normal},
		{"anim", &LvglStyles::anim},
		{"anim_fast", &LvglStyles::anim_fast},
		{"actionBtn", &LvglStyles::actionBtn},
		{"input", &LvglStyles::input},
		{"long_press", &LvglStyles::long_press},
		{"draggable", &LvglStyles::draggable},
		{"dragging", &LvglStyles::dragging},
		{"drag_complete", &LvglStyles::drag_complete},
		{"knob", &LvglStyles::knob},
#if LV_USE_ARC
		{"arc_indic", &LvglStyles::arc_indic},
		{"arc_indic_primary", &LvglStyles::arc_indic_primary},
#endif
#if LV_USE_BAR
		{"bar", &LvglStyles::bar},
		{"bar_indic", &LvglStyles::bar_indic},
#endif
#if LV_USE_BUTTONMATRIX
		{"btnm_bg", &LvglStyles::btnm_bg},
		{"btnm_btn", &LvglStyles::btnm_btn},
		{"btnm_btn_checked", &LvglStyles::btnm_btn_checked},
#endif
#if LV_USE_CHART
		{"chart_series", &LvglStyles::chart_series},
		{"chart_indic", &LvglStyles::chart_indic},
		{"chart_bg", &LvglStyles::chart_bg},
#endif
#if LV_USE_DROPDOWN
		{"dropdown", &LvglStyles::dropdown},
		{"dropdown_list", &LvglStyles::dropdown_list},
#endif
#if LV_USE_CHECKBOX
		{"cb_marker", &LvglStyles::cb_marker},
		{"cb_marker_checked", &LvglStyles::cb_marker_checked},
#endif
#if LV_USE_SWITCH
		{"bg_switch", &LvglStyles::bg_switch},
		{"switch_knob", &LvglStyles::switch_knob},
#endif
#if LV_USE_LINE
		{"line", &LvglStyles::line},
#endif
#if LV_USE_TABLE
		{"table", &LvglStyles::table},
		{"table_cell", &LvglStyles::table_cell},
#endif
#if LV_USE_TEXTAREA
		{"text_area", &LvglStyles::text_area},
		{"ta_cursor", &LvglStyles::ta_cursor},
		{"ta_placeholder", &LvglStyles::ta_placeholder},
#endif
#if LV_USE_CALENDAR
		{"calendar_btnm_bg", &LvglStyles::calendar_btnm_bg},
		{"calendar_btnm_day", &LvglStyles::calendar_btnm_day},
		{"calendar_header", &LvglStyles::calendar_header},
#endif
#if LV_USE_CANVAS
		{"canvas", &LvglStyles::canvas},
#endif
#if LV_USE_MENU
		{"menu_bg", &LvglStyles::menu_bg},
		{"menu_cont", &LvglStyles::menu_cont},
		{"menu_sidebar_cont", &LvglStyles::menu_sidebar_cont},
		{"menu_main_cont", &LvglStyles::menu_main_cont},
		{"menu_page", &LvglStyles::menu_page},
		{"menu_header_cont", &LvglStyles::menu_header_cont},
		{"menu_header_btn", &LvglStyles::menu_header_btn},
		{"menu_section", &LvglStyles::menu_section},
		{"menu_pressed", &LvglStyles::menu_pressed},
		{"menu_separator", &LvglStyles::menu_separator},
#endif
#if LV_USE_MSGBOX
		{"msgbox_backdrop_bg", &LvglStyles::msgbox_backdrop_bg},
#endif
#if LV_USE_KEYBOARD
		{"keyboard_button", &LvglStyles::keyboard_button},
		{"keyboard_button_checked_bg", &LvglStyles::keyboard_button_checked_bg},
#endif
#if LV_USE_LIST
		{"list_bg", &LvglStyles::list_bg},
		{"list_btn", &LvglStyles::list_btn},
		{"list_item_grow", &LvglStyles::list_item_grow},
#endif
#if LV_USE_TABVIEW
		{"tab_bg_focus", &LvglStyles::tab_bg_focus},
		{"tab_btn", &LvglStyles::tab_btn},
#endif
#if LV_USE_LED
		{"led", &LvglStyles::led},
#endif
#if LV_USE_SCALE
		{"scale", &LvglStyles::scale},
#endif
#if LV_USE_SLIDER
		{"slider", &LvglStyles::slider},
		{"slider_indic", &LvglStyles::slider_indic},
		{"slider_knob", &LvglStyles::slider_knob},
#endif
	};

	static constexpr std::pair<const char*, Style ComponentStyles::*> s_componentStyleNames[] = {
		{"sidebar", &ComponentStyles::sidebar},
		{"sidebar_btn", &ComponentStyles::sidebar_btn},
		{"console_btn_anim", &ComponentStyles::console_btn_anim},
		{"estop", &ComponentStyles::estop},
		{"file", &ComponentStyles::file},
		{"folder", &ComponentStyles::folder},
		{"unhomed", &ComponentStyles::unhomed},
		{"temperature_bar", &ComponentStyles::temperature_bar},
		{"temperature_bar_indic", &ComponentStyles::temperature_bar_indic},
		{"tool_list", &ComponentStyles::tool_list},
		{"tool_list_tool", &ComponentStyles::tool_list_tool},
		{"tool_list_heater", &ComponentStyles::tool_list_heater},
		{"tool_selected", &ComponentStyles::tool_selected},
		{"tab_button", &ComponentStyles::tab_button},
		{"slider_buttons", &ComponentStyles::slider_buttons},
		{"list_header", &ComponentStyles::list_header},
		{"list_title", &ComponentStyles::list_title},
		{"graph_legend", &ComponentStyles::graph_legend},
		{"bar_label_bg", &ComponentStyles::bar_label_bg},
		{"bar_label", &ComponentStyles::bar_label},
		{"object_cancel_base", &ComponentStyles::object_cancel_base},
		{"object_cancel_current", &ComponentStyles::object_cancel_current},
		{"object_cancel_cancelled", &ComponentStyles::object_cancel_cancelled},
		{"debug_borders", &ComponentStyles::debug_borders},
	};

	void forEachStyle(const std::function<void(std::string_view name, Style& style)>& fn)
	{
		auto& lvgl = const_cast<LvglStyles&>(getLvglStyles());
		auto& components = const_cast<ComponentStyles&>(getComponentStyles());
		for (const auto& [name, member] : s_lvglStyleNames)
		{
			fn(name, lvgl.*member);
		}
		for (const auto& [name, member] : s_componentStyleNames)
		{
			fn(name, components.*member);
		}
	}

	const Fonts& getFonts()
	{
		ZoneScoped;
		return s_fonts;
	}

	void setFonts(const lv_font_t& header, const lv_font_t& normal, const lv_font_t& emphasis, const lv_font_t& subdued)
	{
		ZoneScoped;
		s_fonts.header = header;
		s_fonts.normal = normal;
		s_fonts.emphasis = emphasis;
		s_fonts.subdued = subdued;
	}

	/**
	 * @brief Callback function to apply theme styles to LVGL objects
	 *
	 * This function applies various styles to LVGL objects based on their type:
	 * - Applies base style to all objects
	 * - Container style to generic objects
	 * - Button style to button objects
	 * - Label style to label objects
	 * - Debug borders in debug mode if enabled
	 *
	 * @param th Pointer to the LVGL theme (unused)
	 * @param obj Pointer to the LVGL object to apply styles to
	 *
	 * @note Function is protected by UI_LOCK()
	 */
	static void applyThemeCb(lv_theme_t* th, lv_obj_t* obj)
	{
		ZoneScoped;
		UI_LOCK();
		LV_UNUSED(th);

		lv_obj_t* parent = lv_obj_get_parent(obj);

		const LvglStyles& lvgl = getLvglStyles();

		lv_obj_add_style(obj, lvgl.base, 0);

		if (parent == NULL)
		{
			lv_obj_add_style(obj, lvgl.screen, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			return;
		}

#if 1
		if (lv_obj_check_type(obj, &lv_obj_class))
		{
#  if LV_USE_TABVIEW
			/*Tabview content area*/
			if (lv_obj_check_type(parent, &lv_tabview_class) && lv_obj_get_child(parent, 1) == obj)
			{
				return;
			}
			/*Tabview button container*/
			else if (lv_obj_check_type(parent, &lv_tabview_class) && lv_obj_get_child(parent, 0) == obj)
			{
				lv_obj_add_style(obj, lvgl.bg_color_list_item, 0);
				lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
				lv_obj_add_style(obj, lvgl.tab_bg_focus, LV_STATE_FOCUS_KEY);
				return;
			}
			/*Tabview pages*/
			else if (lv_obj_check_type(lv_obj_get_parent(parent), &lv_tabview_class))
			{
				lv_obj_add_style(obj, lvgl.pad_normal, 0);
				lv_obj_add_style(obj, lvgl.rotary_scroll, 0);
				lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
				lv_obj_add_style(obj,
								 lvgl.scrollbar_scrolled,
								 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
				return;
			}
#  endif

#  if LV_USE_WIN
			/*Header*/
			if (lv_obj_check_type(parent, &lv_win_class) && lv_obj_get_child(parent, 0) == obj)
			{
				lv_obj_add_style(obj, lvgl.bg_color_header, 0);
				lv_obj_add_style(obj, lvgl.pad_tiny, 0);
				return;
			}
			/*Content*/
			else if (lv_obj_check_type(parent, &lv_win_class) && lv_obj_get_child(parent, 1) == obj)
			{
				lv_obj_add_style(obj, lvgl.screen, 0);
				lv_obj_add_style(obj, lvgl.pad_normal, 0);
				lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
				lv_obj_add_style(obj,
								 lvgl.scrollbar_scrolled,
								 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
				return;
			}
#  endif

#  if LV_USE_CALENDAR
			if (lv_obj_check_type(parent, &lv_calendar_class))
			{
				/*No style*/
				return;
			}
#  endif

			lv_obj_add_style(obj, lvgl.pad_base, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
		}
#  if LV_USE_BUTTON
		else if (lv_obj_check_type(obj, &lv_button_class))
		{

#	if LV_USE_TABVIEW
			lv_obj_t* tv = lv_obj_get_parent(parent); /*parent is the tabview header*/
			if (tv && lv_obj_get_child(tv, 0) == parent)
			{ /*The button is on the tab view header*/
				if (lv_obj_check_type(tv, &lv_tabview_class))
				{
					lv_obj_add_style(obj, lvgl.pressed, LV_STATE_PRESSED);
					lv_obj_add_style(obj, lvgl.bg_color_primary_muted, LV_STATE_CHECKED);
					lv_obj_add_style(obj, lvgl.tab_btn, LV_STATE_CHECKED);
					lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
					lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
					lv_obj_add_style(obj, lvgl.tab_bg_focus, LV_STATE_FOCUS_KEY);
					return;
				}
			}

#	endif
			lv_obj_add_style(obj, lvgl.pad_base, 0);
			lv_obj_add_style(obj, lvgl.btn, 0);
			lv_obj_add_style(obj, lvgl.transition_delayed, 0);
			lv_obj_add_style(obj, lvgl.pressed, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.transition_normal, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.grow, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.btn_checked, LV_STATE_CHECKED);
			lv_obj_add_style(obj, lvgl.disabled, LV_STATE_DISABLED);

#	if LV_USE_MENU
			if (lv_obj_check_type(parent, &lv_menu_sidebar_header_cont_class) ||
				lv_obj_check_type(parent, &lv_menu_main_header_cont_class))
			{
				lv_obj_add_style(obj, lvgl.menu_header_btn, 0);
				lv_obj_add_style(obj, lvgl.menu_pressed, LV_STATE_PRESSED);
			}
#	endif
		}
#  endif

#  if LV_USE_LINE
		else if (lv_obj_check_type(obj, &lv_line_class))
		{
			lv_obj_add_style(obj, lvgl.line, 0);
		}
#  endif

#  if LV_USE_BUTTONMATRIX
		else if (lv_obj_check_type(obj, &lv_buttonmatrix_class))
		{

#	if LV_USE_CALENDAR
			if (lv_obj_check_type(parent, &lv_calendar_class))
			{
				lv_obj_add_style(obj, lvgl.calendar_btnm_bg, 0);
				lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
				lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
				lv_obj_add_style(obj, lvgl.calendar_btnm_day, LV_PART_ITEMS);
				lv_obj_add_style(
					obj, lvgl.pressed, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_PRESSED));
				lv_obj_add_style(
					obj, lvgl.disabled, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_DISABLED));
				lv_obj_add_style(
					obj, lvgl.outline_primary, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_FOCUS_KEY));
				lv_obj_add_style(
					obj, lvgl.outline_secondary, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_EDITED));
				return;
			}
#	endif
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.btnm_bg, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.btn, LV_PART_ITEMS);
			lv_obj_add_style(obj, lvgl.btnm_btn, LV_PART_ITEMS);
			lv_obj_add_style(obj, lvgl.disabled, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_DISABLED));
			lv_obj_add_style(obj, lvgl.pressed, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(
				obj, lvgl.btnm_btn_checked, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(
				obj, lvgl.outline_primary, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_FOCUS_KEY));
			lv_obj_add_style(
				obj, lvgl.outline_secondary, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_EDITED));
		}
#  endif

#  if LV_USE_CANVAS
		else if (lv_obj_check_type(obj, &lv_canvas_class))
		{
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.canvas, LV_PART_MAIN);
		}
#  endif

#  if LV_USE_BAR
		else if (lv_obj_check_type(obj, &lv_bar_class))
		{
			// lv_obj_add_style(obj, lvgl.pad_normal, 0);
			lv_obj_add_style(obj, lvgl.bar, 0);
			lv_obj_add_style(obj, lvgl.anim, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.bg_color_primary, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.bar_indic, LV_PART_INDICATOR);
		}
#  endif

#  if LV_USE_SLIDER
		else if (lv_obj_check_type(obj, &lv_slider_class))
		{
			lv_obj_add_style(obj, lvgl.bg_color_primary_muted, 0);
			lv_obj_add_style(obj, lvgl.slider, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.bg_color_primary_muted, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.slider_indic, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.knob, LV_PART_KNOB);
			lv_obj_add_style(obj, lvgl.slider_knob, LV_PART_KNOB);
			lv_obj_add_style(obj, lvgl.grow, static_cast<int>(LV_PART_KNOB) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(obj, lvgl.transition_delayed, LV_PART_KNOB);
			lv_obj_add_style(
				obj, lvgl.transition_normal, static_cast<int>(LV_PART_KNOB) | static_cast<int>(LV_STATE_PRESSED));
		}
#  endif

#  if LV_USE_TABLE
		else if (lv_obj_check_type(obj, &lv_table_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_zero, 0);
			lv_obj_add_style(obj, lvgl.table, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			lv_obj_add_style(obj, lvgl.table_cell, LV_PART_ITEMS);
			lv_obj_add_style(obj, lvgl.pad_normal, LV_PART_ITEMS);
			lv_obj_add_style(obj, lvgl.pressed, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(
				obj, lvgl.bg_color_primary, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_FOCUS_KEY));
			lv_obj_add_style(
				obj, lvgl.bg_color_secondary, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_EDITED));
		}
#  endif

#  if LV_USE_CHECKBOX
		else if (lv_obj_check_type(obj, &lv_checkbox_class))
		{
			lv_obj_add_style(obj, lvgl.pad_base, 0);
			lv_obj_add_style(obj, lvgl.pad_gap, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(
				obj, lvgl.disabled, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_DISABLED));
			lv_obj_add_style(obj, lvgl.text, 0);
			lv_obj_add_style(obj, lvgl.cb_marker, LV_PART_INDICATOR);
			lv_obj_add_style(
				obj, lvgl.bg_color_primary, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(
				obj, lvgl.cb_marker_checked, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(
				obj, lvgl.pressed, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(obj, lvgl.grow, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(
				obj, lvgl.transition_normal, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(obj, lvgl.transition_delayed, LV_PART_INDICATOR);
		}
#  endif

#  if LV_USE_SWITCH
		else if (lv_obj_check_type(obj, &lv_switch_class))
		{
			lv_obj_add_style(obj, lvgl.pad_base, 0);
			lv_obj_add_style(obj, lvgl.pad_gap, 0);
			lv_obj_add_style(obj, lvgl.bg_switch, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.anim_fast, 0);
			lv_obj_add_style(obj, lvgl.disabled, LV_STATE_DISABLED);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(
				obj, lvgl.bg_color_primary, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(obj, lvgl.knob, LV_PART_KNOB);
			lv_obj_add_style(obj, lvgl.switch_knob, LV_PART_KNOB);

			lv_obj_add_style(
				obj, lvgl.transition_normal, static_cast<int>(LV_PART_INDICATOR) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(obj, lvgl.transition_normal, LV_PART_INDICATOR);
		}
#  endif

#  if LV_USE_CHART
		else if (lv_obj_check_type(obj, &lv_chart_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_small, 0);
			lv_obj_add_style(obj, lvgl.chart_bg, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			lv_obj_add_style(obj, lvgl.chart_series, LV_PART_ITEMS);
			lv_obj_add_style(obj, lvgl.chart_indic, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.chart_series, LV_PART_CURSOR);
		}
#  endif

#  if LV_USE_ROLLER
		else if (lv_obj_check_type(obj, &lv_roller_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.anim, 0);
			lv_obj_add_style(obj, lvgl.line_space_large, 0);
			lv_obj_add_style(obj, lvgl.text_align_center, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.bg_color_primary, LV_PART_SELECTED);
		}
#  endif

#  if LV_USE_DROPDOWN
		else if (lv_obj_check_type(obj, &lv_dropdown_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_small, 0);
			lv_obj_add_style(obj, lvgl.transition_delayed, 0);
			lv_obj_add_style(obj, lvgl.input, 0);
			lv_obj_add_style(obj, lvgl.dropdown, 0);
			lv_obj_add_style(obj, lvgl.transition_normal, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.pressed, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.transition_normal, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.disabled, LV_STATE_DISABLED);
			[[maybe_unused]] const int32_t pad = lv_obj_get_style_pad_top(obj, LV_PART_MAIN);
			LOG_DBG("Pad {:d}", pad);
		}
		else if (lv_obj_check_type(obj, &lv_dropdownlist_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.clip_corner, 0);
			lv_obj_add_style(obj, lvgl.line_space_large, 0);
			lv_obj_add_style(obj, lvgl.text, 0);
			lv_obj_add_style(obj, lvgl.dropdown_list, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			lv_obj_add_style(
				obj, lvgl.bg_color_primary, static_cast<int>(LV_PART_SELECTED) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(
				obj, lvgl.pressed, static_cast<int>(LV_PART_SELECTED) | static_cast<int>(LV_STATE_PRESSED));
		}
#  endif

#  if LV_USE_ARC
		else if (lv_obj_check_type(obj, &lv_arc_class))
		{
			lv_obj_add_style(obj, lvgl.arc_indic, 0);
			lv_obj_add_style(obj, lvgl.arc_indic, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.arc_indic_primary, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.knob, LV_PART_KNOB);
		}
#  endif

#  if LV_USE_ARCLABEL
		else if (lv_obj_check_type(obj, &lv_arclabel_class))
		{
			lv_obj_add_style(obj, lvgl.arc_indic, 0);
			lv_obj_add_style(obj, lvgl.arc_indic, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.arc_indic_primary, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.knob, LV_PART_KNOB);
		}
#  endif

#  if LV_USE_SPINNER
		else if (lv_obj_check_type(obj, &lv_spinner_class))
		{
			lv_obj_add_style(obj, lvgl.arc_indic, 0);
			lv_obj_add_style(obj, lvgl.arc_indic, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.arc_indic_primary, LV_PART_INDICATOR);
		}
#  endif

#  if LV_USE_TEXTAREA
		else if (lv_obj_check_type(obj, &lv_textarea_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_base, 0); // normally overridden by `input` or `text_area` styles
			lv_obj_add_style(obj, lvgl.input, 0);
			lv_obj_add_style(obj, lvgl.text_area, 0);
			lv_obj_add_style(obj, lvgl.disabled, LV_STATE_DISABLED);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			lv_obj_add_style(
				obj, lvgl.ta_cursor, static_cast<int>(LV_PART_CURSOR) | static_cast<int>(LV_STATE_FOCUSED));
			lv_obj_add_style(obj, lvgl.ta_placeholder, LV_PART_TEXTAREA_PLACEHOLDER);
		}
#  endif

#  if LV_USE_CALENDAR
		else if (lv_obj_check_type(obj, &lv_calendar_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_zero, 0);
		}

#	if LV_USE_CALENDAR_HEADER_ARROW
		else if (lv_obj_check_type(obj, &lv_calendar_header_arrow_class))
		{
			lv_obj_add_style(obj, lvgl.calendar_header, 0);
		}
#	endif

#	if LV_USE_CALENDAR_HEADER_DROPDOWN
		else if (lv_obj_check_type(obj, &lv_calendar_header_dropdown_class))
		{
			lv_obj_add_style(obj, lvgl.calendar_header, 0);
		}
#	endif
#  endif

#  if LV_USE_KEYBOARD
		else if (lv_obj_check_type(obj, &lv_keyboard_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_small, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.disabled, static_cast<int>(LV_PART_ITEMS) | LV_STATE_DISABLED);
			lv_obj_add_style(obj, lvgl.text, 0);
			lv_obj_add_style(obj, lvgl.keyboard_button, LV_PART_ITEMS);
			lv_obj_add_style(obj, lvgl.pressed, static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_PRESSED));
			lv_obj_add_style(obj,
							 lvgl.keyboard_button_checked_bg,
							 static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(obj,
							 lvgl.bg_color_primary_muted,
							 static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_FOCUS_KEY));
			lv_obj_add_style(obj,
							 lvgl.bg_color_secondary_muted,
							 static_cast<int>(LV_PART_ITEMS) | static_cast<int>(LV_STATE_EDITED));
		}
#  endif

#  if LV_USE_LABEL && LV_USE_TEXTAREA
		else if (lv_obj_check_type(obj, &lv_label_class) && lv_obj_check_type(parent, &lv_textarea_class))
		{
			lv_obj_add_style(obj, lvgl.bg_color_primary, LV_PART_SELECTED);
		}
#  endif

#  if LV_USE_LIST
		else if (lv_obj_check_type(obj, &lv_list_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_normal, 0);
			lv_obj_add_style(obj, lvgl.list_bg, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			return;
		}
		else if (lv_obj_check_type(obj, &lv_list_text_class))
		{
			lv_obj_add_style(obj, lvgl.text_header, 0);
			lv_obj_add_style(obj, lvgl.bg_color_header, 0);
			lv_obj_add_style(obj, lvgl.list_item_grow, 0);
			lv_obj_add_style(obj, lvgl.pad_small, 0);
		}
		else if (lv_obj_check_type(obj, &lv_list_button_class))
		{
			lv_obj_add_style(obj, lvgl.bg_color_list_item, 0);
			lv_obj_add_style(obj, lvgl.list_btn, 0);
			lv_obj_add_style(obj, lvgl.pad_normal, 0);
			lv_obj_add_style(obj, lvgl.bg_color_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.list_item_grow, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.list_item_grow, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.pressed, LV_STATE_PRESSED);
		}
#  endif
#  if LV_USE_MENU
		else if (lv_obj_check_type(obj, &lv_menu_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.menu_bg, 0);
		}
		else if (lv_obj_check_type(obj, &lv_menu_sidebar_cont_class))
		{
			lv_obj_add_style(obj, lvgl.menu_sidebar_cont, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
		}
		else if (lv_obj_check_type(obj, &lv_menu_main_cont_class))
		{
			lv_obj_add_style(obj, lvgl.menu_main_cont, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
		}
		else if (lv_obj_check_type(obj, &lv_menu_cont_class))
		{
			lv_obj_add_style(obj, lvgl.menu_cont, 0);
			lv_obj_add_style(obj, lvgl.menu_pressed, LV_STATE_PRESSED);
			lv_obj_add_style(obj,
							 lvgl.bg_color_primary_muted,
							 static_cast<int>(LV_STATE_PRESSED) | static_cast<int>(LV_STATE_CHECKED));
			lv_obj_add_style(obj, lvgl.bg_color_primary_muted, LV_STATE_CHECKED);
			lv_obj_add_style(obj, lvgl.bg_color_primary, LV_STATE_FOCUS_KEY);
		}
		else if (lv_obj_check_type(obj, &lv_menu_sidebar_header_cont_class) ||
				 lv_obj_check_type(obj, &lv_menu_main_header_cont_class))
		{
			lv_obj_add_style(obj, lvgl.menu_header_cont, 0);
		}
		else if (lv_obj_check_type(obj, &lv_menu_page_class))
		{
			lv_obj_add_style(obj, lvgl.menu_page, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
		}
		else if (lv_obj_check_type(obj, &lv_menu_section_class))
		{
			lv_obj_add_style(obj, lvgl.menu_section, 0);
		}
		else if (lv_obj_check_type(obj, &lv_menu_separator_class))
		{
			lv_obj_add_style(obj, lvgl.menu_separator, 0);
		}
#  endif
#  if LV_USE_MSGBOX
		else if (lv_obj_check_type(obj, &lv_msgbox_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.clip_corner, 0);
			return;
		}
		else if (lv_obj_check_type(obj, &lv_msgbox_backdrop_class))
		{
			lv_obj_add_style(obj, lvgl.msgbox_backdrop_bg, 0);
			return;
		}
		else if (lv_obj_check_type(obj, &lv_msgbox_header_class))
		{
			lv_obj_add_style(obj, lvgl.pad_tiny, 0);
			lv_obj_add_style(obj, lvgl.bg_color_header, 0);
			return;
		}
		else if (lv_obj_check_type(obj, &lv_msgbox_footer_class))
		{
			lv_obj_add_style(obj, lvgl.pad_tiny, 0);
			return;
		}
		else if (lv_obj_check_type(obj, &lv_msgbox_content_class))
		{
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
			lv_obj_add_style(obj, lvgl.pad_tiny, 0);
			return;
		}
		else if (lv_obj_check_type(obj, &lv_msgbox_header_button_class) ||
				 lv_obj_check_type(obj, &lv_msgbox_footer_button_class))
		{
			lv_obj_add_style(obj, lvgl.btn, 0);
			lv_obj_add_style(obj, lvgl.bg_color_primary, 0);
			lv_obj_add_style(obj, lvgl.transition_delayed, 0);
			lv_obj_add_style(obj, lvgl.pressed, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.transition_normal, LV_STATE_PRESSED);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.bg_color_secondary, LV_STATE_CHECKED);
			lv_obj_add_style(obj, lvgl.disabled, LV_STATE_DISABLED);
			return;
		}

#  endif
#  if LV_USE_SPAN
		else if (lv_obj_check_type(obj, &lv_spangroup_class))
		{
			lv_obj_add_style(obj, lvgl.text, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
		}
#  endif
#  if LV_USE_SPINBOX
		else if (lv_obj_check_type(obj, &lv_spinbox_class))
		{
			lv_obj_add_style(obj, lvgl.card, 0);
			lv_obj_add_style(obj, lvgl.pad_small, 0);
			lv_obj_add_style(obj, lvgl.outline_primary, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(obj, lvgl.outline_secondary, LV_STATE_EDITED);
			lv_obj_add_style(obj, lvgl.bg_color_primary, LV_PART_CURSOR);
		}
#  endif
#  if LV_USE_TILEVIEW
		else if (lv_obj_check_type(obj, &lv_tileview_class))
		{
			lv_obj_add_style(obj, lvgl.screen, 0);
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
		}
		else if (lv_obj_check_type(obj, &lv_tileview_tile_class))
		{
			lv_obj_add_style(obj, lvgl.scrollbar, LV_PART_SCROLLBAR);
			lv_obj_add_style(obj,
							 lvgl.scrollbar_scrolled,
							 static_cast<int>(LV_PART_SCROLLBAR) | static_cast<int>(LV_STATE_SCROLLED));
		}
#  endif

#  if LV_USE_TABVIEW
		else if (lv_obj_check_type(obj, &lv_tabview_class))
		{
			lv_obj_add_style(obj, lvgl.screen, 0);
			lv_obj_add_style(obj, lvgl.pad_zero, 0);
		}
#  endif

#  if LV_USE_WIN
		else if (lv_obj_check_type(obj, &lv_win_class))
		{
			lv_obj_add_style(obj, lvgl.clip_corner, 0);
		}
#  endif

#  if LV_USE_LED
		else if (lv_obj_check_type(obj, &lv_led_class))
		{
			lv_obj_add_style(obj, lvgl.led, 0);
		}
#  endif

#  if LV_USE_SCALE
		else if (lv_obj_check_type(obj, &lv_scale_class))
		{
			lv_obj_add_style(obj, lvgl.scale, LV_PART_MAIN);
			lv_obj_add_style(obj, lvgl.scale, LV_PART_INDICATOR);
			lv_obj_add_style(obj, lvgl.scale, LV_PART_ITEMS);
		}
#  endif

#  if DEBUG_BORDERS
		const auto& components = getComponentStyles();

		if (lv_obj_has_style(lv_screen_active(), components.debug_borders))
		{
			// Add debug borders to any newly created objects
			lv_obj_add_style(obj, components.debug_borders, LV_PART_MAIN);
		}
#  endif

#  if USE_LV_ANIMATION == 0
		// Disable animations globally
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_MAIN);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_SCROLLBAR);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_INDICATOR);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_KNOB);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_SELECTED);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_ITEMS);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_CURSOR);
		lv_obj_set_style_anim_duration(obj, 0, LV_PART_CUSTOM_FIRST);
#  endif
#endif
	}

	void init(lv_display_t* display)
	{
		ZoneScoped;
		UI_LOCK();
		LOG_INFO("Initializing themes");

		if (display == NULL)
		{
			display = lv_display_get_default();
		}

		// Initialize uninitialized styles
		for (auto& style : uninitializedStyles())
		{
			style->init();
		}
		uninitializedStyles().clear();

		s_lvglStyles.reset();
		s_componentStyles.reset();

		// lv_theme_t* baseTheme = lv_theme_default_init(display,
		// 											  lv_palette_main(LV_PALETTE_BLUE),
		// 											  lv_palette_main(LV_PALETTE_RED),
		// 											  LV_THEME_DEFAULT_DARK,
		// 											  &lv_font_montserrat_14);

		/*Initialize the new theme from the current theme*/
		lv_theme_t* th_act = lv_display_get_theme(display);
		s_theme = *th_act;

		/*Set the parent theme and the style apply callback for the new theme*/
		lv_theme_set_parent(&s_theme, th_act);
		lv_theme_set_apply_cb(&s_theme, applyThemeCb);

		/*Assign the new theme to the current display*/
		lv_display_set_theme(display, &s_theme);

		lv_theme_apply(lv_screen_active());

		// The styles are built when the XML UI loads the themes (see Theme.h)

		/* Set icon folder */
		std::string_view iconFolder = StorageHelper::getData(ID_ICON_FOLDER);
		setIconFolder(iconFolder);

#if DEBUG_BORDERS
		bool debugBordersEnabeled = StorageHelper::getData(ID_DEBUG_BORDERS);
		showDebugBorders(lv_screen_active(), debugBordersEnabeled);
#endif
	}

	static std::string s_iconFolder(DEFAULT_ICON_SET);

	void resetIconFolder()
	{
		ZoneScoped;
		UI_LOCK();
		setIconFolder(DEFAULT_ICON_SET);
	}

	const std::string& getIconFolder()
	{
		ZoneScoped;
		UI_LOCK();
		return s_iconFolder;
	}

	void setIconFolder(std::string_view folder)
	{
		ZoneScoped;
		UI_LOCK();
		if (s_iconFolder == folder)
			return;

		LOG_INFO("Setting icon folder to {:s}", folder);

		auto sets = getIconSets();
		if (std::find(sets.begin(), sets.end(), folder) == sets.end())
		{
			LOG_WARN("Icon set {:s} not found, keeping current icon set {:s}", folder, s_iconFolder);
			return;
		}

		s_iconFolder = folder;
		StorageHelper::setData(ID_ICON_FOLDER, s_iconFolder);
		lv_obj_refresh(lv_screen_active());
	}

	std::vector<std::string> getIconSets()
	{
		ZoneScoped;
		std::vector<std::string> sets;
		for (const auto& entry : std::filesystem::directory_iterator(ASSETS_FOLDER "icons/"))
		{
			if (!entry.is_directory())
				continue;
			if (entry.path().filename() == "examples" || entry.path().filename() == "hardware_test")
				continue;
			sets.push_back(entry.path().filename().string());
		}
		return sets;
	}

	std::string getIconPath(std::string_view icon_name)
	{
		ZoneScoped;
		return getFixedIconPath(s_iconFolder, icon_name);
	}

	std::string getFixedIconPath(std::string_view folder, std::string_view icon_name)
	{
		ZoneScoped;
		UI_LOCK();
		return fmt::format(ASSETS_FOLDER "icons/{:s}/{:s}", folder, icon_name);
	}

	bool iconExists(std::string_view icon_name)
	{
		ZoneScoped;
		std::string path = getIconPath(icon_name);
		return std::filesystem::exists(path);
	}

#if DEBUG_BORDERS
	bool isdebugBorderVisible(lv_obj_t* obj)
	{
		ZoneScoped;
		UI_LOCK();
		return lv_obj_has_style(obj, getComponentStyles().debug_borders);
	}

	static void _showDebugBorders(lv_obj_t* obj, const bool show, const bool recursive)
	{
		ZoneScoped;
		if (show)
		{
			lv_obj_add_style(obj, getComponentStyles().debug_borders, LV_PART_MAIN, recursive);
		}
		else
		{
			lv_obj_remove_style(obj, getComponentStyles().debug_borders, LV_PART_MAIN, recursive);
		}
	}

	void showDebugBorders(lv_obj_t* obj, const bool show, const bool recursive)
	{
		ZoneScoped;
		UI_LOCK();
		_showDebugBorders(obj, show, recursive);
		lv_obj_refresh_style(obj, LV_PART_ANY, LV_STYLE_PROP_ANY);
	}
#endif

} // namespace UI::Themes

/**
 * @brief Send LV_EVENT_REFRESH to obj and all its children recursively
 * @param obj
 */
void lv_obj_refresh(lv_obj_t* obj)
{
	lv_obj_tree_walk(
		obj,
		[](lv_obj_t* obj, void*)
		{
			lv_obj_send_event(obj, LV_EVENT_REFRESH, NULL);
			return LV_OBJ_TREE_WALK_NEXT;
		},
		NULL);
}

bool lv_obj_has_style(lv_obj_t* obj, const lv_style_t* style)
{
	ZoneScoped;
	UI_LOCK();

	for (size_t i = 0; i < obj->style_cnt; i++)
	{
		if (obj->styles[i].style == style)
		{
			return true;
		}
	}
	return false;
}

void lv_obj_add_style(lv_obj_t* obj, const lv_style_t* style, const lv_style_selector_t selector, const bool recursive)
{
	ZoneScoped;
	UI_LOCK();
	if (recursive)
	{
		uint32_t childCount = lv_obj_get_child_count(obj);
		for (uint32_t i = 0; i < childCount; ++i)
		{
			lv_obj_t* child = lv_obj_get_child(obj, i);
			lv_obj_add_style(child, style, selector, true);
		}
	}

	// Raw lvgl call
	lv_obj_add_style(obj, style, selector);
}

void lv_obj_remove_style(lv_obj_t* obj,
						 const lv_style_t* style,
						 const lv_style_selector_t selector,
						 const bool recursive)
{
	ZoneScoped;
	UI_LOCK();
	if (recursive)
	{
		uint32_t childCount = lv_obj_get_child_count(obj);
		for (uint32_t i = 0; i < childCount; ++i)
		{
			lv_obj_t* child = lv_obj_get_child(obj, i);
			lv_obj_remove_style(child, style, selector, true);
		}
	}

	// Raw lvgl call
	lv_obj_remove_style(obj, style, selector);
}

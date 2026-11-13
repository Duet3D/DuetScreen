/*
 * Styles.h
 *
 *  Created on: 2025-01-21
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Styles/Font.h"
#include "lvgl/lvgl.h"
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#if SIMULATION
#  define ASSETS_FOLDER "assets/"
#else
#  define ASSETS_FOLDER "/etc/assets/"
#endif

namespace UI::Themes
{
	/** An LVGL style. The global ones (LvglStyles, ComponentStyles) are filled in by the active theme. */
	struct Style
	{
		/** An empty, unnamed style */
		Style();
		/** An empty style called `name` */
		Style(const char* name);
		/** A style filled in by `initFunc`, now or, if LVGL isn't running yet, by Themes::init() */
		Style(const char* name, std::function<void(lv_style_t*)> initFunc);

		/** Copy the properties of `other` */
		Style(const Style&);
		/** Replace the properties with those of `other` */
		Style& operator=(const Style&);

		/** Take over the properties of `other` */
		Style(Style&&);
		/** Take over the properties of `other` */
		Style& operator=(Style&&);

		/** Free the properties */
		~Style();

		/** Run the deferred initFunc */
		void init();

		/** Set one property */
		void setProp(lv_style_prop_t prop, lv_style_value_t value) { lv_style_set_prop(&style, prop, value); }
		/** Read one property */
		lv_style_res_t getProp(lv_style_prop_t prop, lv_style_value_t* value) const
		{
			return lv_style_get_prop(&style, prop, value);
		}
		/** @return whether the style sets `prop` */
		bool hasProp(lv_style_prop_t prop) const
		{
			lv_style_value_t v;
			return getProp(prop, &v) == LV_STYLE_RES_FOUND;
		}

		/** The LVGL style, to add to a widget */
		operator lv_style_t*() { return &style; }
		/** The LVGL style, to add to a widget */
		operator const lv_style_t*() const { return &style; }

	  private:
		lv_style_t style;							// The LVGL style
		bool m_moved = false;						// Moved from: don't free the properties
		const char* name;							// Name, for debugging
		std::function<void(lv_style_t*)> initFunc; // Deferred initialisation, or empty
	};

	/** Styles for LVGL's own widgets, and general-purpose ones. A theme sets their properties by member name. */
	struct LvglStyles
	{
		/** Empty styles; the active theme fills them in */
		LvglStyles();

		Style base; // Base style applied to all objects

		/* Background */
		Style bg_dark; // Screen background colour
		Style bg; // Card background colour
		Style bg_light; // Raised background colour
		Style bg_color_primary; // button, button matrix (checked), bar indicator, slider indicator, table part focus
								// key, checkbox indicator, switch indicator, roller selected, dropdown list selected,
								// textarea selected, list button focus key, spinbox cursor, msgbox buttons
		Style bg_color_primary_muted; // Muted primary background
		Style bg_color_secondary; // Secondary (action) background
		Style bg_color_secondary_muted; // Muted secondary background
		Style bg_color_success; // Success background
		Style bg_color_warning; // Warning background
		Style bg_color_error; // Error background
		Style bg_color_header; // Translucent header strip
		Style bg_color_list_item; // List item background
		Style bg_modal; // Translucent backdrop behind modals

		/* Text */
		Style text; // Body text
		Style text_muted; // Secondary text
		Style text_emphasis; // Emphasised text
		Style text_header; // Headings
		Style text_warning; // Warnings

		/* Border */
		Style border; // Standard border
		Style border_muted; // Subtle border
		Style border_highlight; // Bright border
		Style border_color_primary; // Primary-coloured border
		Style border_color_secondary; // Secondary-coloured border
		Style border_color_card; // Card border colour

		Style screen; // Screen background
		Style scrollbar; // Scrollbars
		Style scrollbar_scrolled; // Scrollbars while scrolling
		Style card; // Cards and panels
		Style tab_card; // Tab page panels
		Style modal; // Modal dialogs
		Style btn; // Buttons
		Style btn_checked; // Checked buttons

		/* Icons */
		Style icon; // Icons
		Style icon_recolor; // Icons tinted with the text colour

		/*Utility*/
		Style pressed; // Pressed widgets
		Style disabled; // Disabled widgets
		Style pad_base; // Default padding
		Style pad_zero; // No padding
		Style pad_tiny; // Tiny padding
		Style pad_small; // Small padding
		Style pad_normal; // Normal padding
		Style pad_large; // Large padding
		Style pad_gap; // Gap between children
		Style line_space_large;	 // roller, dropdown
		Style text_align_center; // roller
		Style outline_primary;	 // Focused via keypad or encoder
		Style outline_secondary; // Edited by an encoder
		Style circle; // Fully rounded corners
		Style no_radius; // Square corners
		Style no_border; // No border
		Style clip_corner; // Clip children to rounded corners
		Style rotary_scroll; // Scroll speed for rotary encoders
		Style grow; // Grow slightly (pressed buttons)
		Style transition_delayed; // Delayed transitions
		Style transition_normal; // Normal transitions
		Style anim; // Normal animation time
		Style anim_fast; // Fast animation time

		Style action_btn; // UI elements that perform actions on the Duet
		Style input; // Text inputs
		Style long_press; // Long-press feedback
		Style draggable; // Draggable widgets
		Style dragging; // Widgets being dragged
		Style drag_complete; // Dragged to the end

		/*Parts*/
		Style knob; // Slider and switch knobs

#if LV_USE_ARC
		Style arc_indic; // Arc indicator
		Style arc_indic_primary; // Primary-coloured arc indicator
#endif

#if LV_USE_BAR
		Style bar; // Bar background
		Style bar_indic; // Bar indicator
#endif

#if LV_USE_BUTTONMATRIX
		Style btnm_bg; // Button matrix background
		Style btnm_btn; // Button matrix buttons
		Style btnm_btn_checked; // Checked button matrix buttons
#endif

#if LV_USE_CHART
		Style chart_series; // Chart series lines
		Style chart_indic; // Chart point markers
		Style chart_bg; // Chart background and grid
#endif

#if LV_USE_DROPDOWN
		Style dropdown; // Dropdowns
		Style dropdown_list; // Open dropdown lists
#endif

#if LV_USE_CHECKBOX
		Style cb_marker; // Checkbox box
		Style cb_marker_checked; // Checkbox tick
#endif

#if LV_USE_SWITCH
		Style bg_switch; // Switch track
		Style switch_knob; // Switch knob
#endif

#if LV_USE_LINE
		Style line; // Lines
#endif

#if LV_USE_TABLE
		Style table; // Tables
		Style table_cell; // Table cells
#endif

#if LV_USE_TEXTAREA
		Style text_area; // Text areas
		Style ta_cursor; // Text area cursor
		Style ta_placeholder; // Text area placeholder
#endif

#if LV_USE_CALENDAR
		Style calendar_btnm_bg; // Calendar day grid
		Style calendar_btnm_day; // Calendar days
		Style calendar_header; // Calendar header
#endif

#if LV_USE_CANVAS
		Style canvas; // Canvases
#endif

#if LV_USE_MENU
		Style menu_bg; // Menu background
		Style menu_cont; // Menu items
		Style menu_sidebar_cont; // Menu sidebar
		Style menu_main_cont; // Menu main area
		Style menu_page; // Menu pages
		Style menu_header_cont; // Menu header
		Style menu_header_btn; // Menu header buttons
		Style menu_section; // Menu sections
		Style menu_pressed; // Pressed menu items
		Style menu_separator; // Menu separators
#endif

#if LV_USE_MSGBOX
		Style msgbox_backdrop_bg; // Message box backdrop
#endif

#if LV_USE_KEYBOARD
		Style keyboard_button; // Keyboard keys
		Style keyboard_button_checked_bg; // Checked keyboard keys
#endif

#if LV_USE_LIST
		Style list_bg; // Lists
		Style list_btn; // List buttons
		Style list_item_grow; // Grow a pressed list item
#endif

#if LV_USE_TABVIEW
		Style tab_bg_focus; // Focused tab bar
		Style tab_btn; // Selected tab button
#endif
#if LV_USE_LED
		Style led; // LEDs
#endif

#if LV_USE_SCALE
		Style scale; // Scales
#endif

#if LV_USE_SLIDER
		Style slider; // Slider track
		Style slider_indic; // Slider fill
		Style slider_knob; // Slider knob
#endif
	};

	/** Styles for DuetScreen's own components. A theme sets their properties by member name. */
	struct ComponentStyles
	{
		/* Sidebar */
		Style sidebar;				 // Sidebar style
		Style sidebar_btn;			 // Sidebar button style
		Style console_btn_anim;		 // Console button animation style
		Style estop;				 // Emergency stop button style

		/* Files */
		Style file; // File items
		Style folder; // Folder items

		/* Move */
		Style unhomed; // Unhomed axes in the move view

		/* Temperature */
		Style temperature_bar; // Temperature bar
		Style temperature_bar_indic; // Temperature bar marker
		Style tool_list; // Tool list
		Style tool_list_tool; // One tool in the tool list
		Style tool_list_heater; // One heater in the tool list
		Style tool_selected; // The selected tool

		/* Tabview */
		Style tab_button;			 // Used in tabview tabs

		/* Slider */
		Style slider_buttons;		 // Slider increment/decrement buttons

		/* List */
		Style list_header; // List header strip
		Style list_title; // List title text

		/* Graph */
		Style graph_legend; // Graph legend

		/* Bar */
		Style bar_label_bg; // Background behind a bar's label
		Style bar_label; // A bar's label

		/* Object cancel */
		Style object_cancel_base;	   // Base style for object cancel list items
		Style object_cancel_current;   // Style for the current object label in the object cancel view
		Style object_cancel_cancelled; // Style for the cancelled object label in the object cancel view

		Style debug_borders; // Outlines every widget when debug borders are shown (DEBUG_BORDERS builds)
	};

	/** The global LVGL widget styles */
	const LvglStyles& getLvglStyles();
	/** The global component styles */
	const ComponentStyles& getComponentStyles();

	/**
	 * Call `fn(name, style)` for every global style (LvglStyles and ComponentStyles). The name is the member's name,
	 * which is also how XML and theme files refer to the style.
	 */
	void forEachStyle(const std::function<void(std::string_view name, Style& style)>& fn);

	/**
	 * The theme fonts. Styles point at these objects (`lv_style_set_text_font(style, &getFonts().normal)`), and the
	 * active theme copies new fonts into them, so a theme or typeface change reaches every style using them.
	 */
	struct Fonts
	{
		lv_font_t header;	// Headings
		lv_font_t normal;	// Body text
		lv_font_t emphasis; // Emphasised text
		lv_font_t subdued;	// Secondary text
	};

	/** The theme fonts */
	const Fonts& getFonts();

	/** Copy the active theme's fonts into the theme fonts (see Theme.cpp) */
	void setFonts(const lv_font_t& header, const lv_font_t& normal, const lv_font_t& emphasis, const lv_font_t& subdued);

	/**
	 * Install DuetScreen's LVGL theme on `display`: the callback that gives every new widget its styles. The styles
	 * themselves are built by the active theme (see Theme.h), which the XML UI loads.
	 */
	void init(lv_display_t* display);

	/* Icons */
	void resetIconFolder();
	const std::string& getIconFolder();
	void setIconFolder(std::string_view folder);
	std::vector<std::string> getIconSets();
	std::string getIconPath(std::string_view icon_name);
	std::string getFixedIconPath(std::string_view folder, std::string_view icon_name);
	bool iconExists(std::string_view icon_name);

#if DEBUG_BORDERS
	bool isdebugBorderVisible(lv_obj_t* obj);
	void showDebugBorders(lv_obj_t* obj, const bool show, const bool recursive = true);
#endif
} // namespace UI::Themes

void lv_obj_refresh(lv_obj_t*);
bool lv_obj_has_style(lv_obj_t* obj, const lv_style_t* style);
void lv_obj_add_style(lv_obj_t* obj, const lv_style_t* style, lv_style_selector_t selector, const bool recursive);
void lv_obj_remove_style(lv_obj_t* obj, const lv_style_t* style, lv_style_selector_t selector, const bool recursive);

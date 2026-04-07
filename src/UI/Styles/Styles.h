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
	struct Style
	{
		Style();
		Style(const char* name);
		Style(const char* name, std::function<void(lv_style_t*)> initFunc);

		/* Copy */
		Style(const Style&);
		Style& operator=(const Style&);

		/* Move */
		Style(Style&&);
		Style& operator=(Style&&);

		~Style();

		void init();

		void setProp(lv_style_prop_t prop, lv_style_value_t value) { lv_style_set_prop(&style, prop, value); }
		lv_style_res_t getProp(lv_style_prop_t prop, lv_style_value_t* value) const
		{
			return lv_style_get_prop(&style, prop, value);
		}
		bool hasProp(lv_style_prop_t prop) const
		{
			lv_style_value_t v;
			return getProp(prop, &v) == LV_STYLE_RES_FOUND;
		}

		operator lv_style_t*() { return &style; }
		operator const lv_style_t*() const { return &style; }

	  private:
		lv_style_t style;
		bool m_moved = false;
		const char* name;
		std::function<void(lv_style_t*)> initFunc;
	};

	struct LvglStyles
	{
		LvglStyles();

		Style base; // Base style applied to all objects

		/* Background */
		Style bg_dark;
		Style bg;
		Style bg_light;
		Style bg_color_primary; // button, button matrix (checked), bar indicator, slider indicator, table part focus
								// key, checkbox indicator, switch indicator, roller selected, dropdown list selected,
								// textarea selected, list button focus key, spinbox cursor, msgbox buttons
		Style bg_color_primary_muted;
		Style bg_color_secondary;
		Style bg_color_secondary_muted;
		Style bg_color_success;
		Style bg_color_warning;
		Style bg_color_error;
		Style bg_color_header;
		Style bg_color_list_item;
		Style bg_modal;

		/* Text */
		Style text;
		Style text_muted;
		Style text_emphasis;
		Style text_header;
		Style text_warning;

		/* Border */
		Style border;
		Style border_muted;
		Style border_highlight;
		Style border_color_primary;
		Style border_color_secondary;
		Style border_color_card;

		Style screen;
		Style scrollbar;
		Style scrollbar_scrolled;
		Style card;
		Style tab_card;
		Style modal;
		Style btn;
		Style btn_checked;

		/* Shadow */
		Style shadow_raised;
		Style shadow_lowered;

		/* Icons */
		Style icon;
		Style icon_recolor;

		/*Utility*/
		Style pressed;
		Style disabled;
		Style pad_base;
		Style pad_zero;
		Style pad_tiny;
		Style pad_small;
		Style pad_normal;
		Style pad_large;
		Style pad_gap;
		Style line_space_large;	 // roller, dropdown
		Style text_align_center; // roller
		Style outline_primary;	 // Focused via keypad or encoder
		Style outline_secondary; // Edited by an encoder
		Style circle;
		Style no_radius;
		Style no_border;
		Style clip_corner;
		Style rotary_scroll;
		Style grow;
		Style transition_delayed;
		Style transition_normal;
		Style anim;
		Style anim_fast;

		Style actionBtn; // for UI elements that perform actions on the Duet
		Style input;
		Style long_press;
		Style draggable;
		Style dragging;
		Style drag_complete;

		/*Parts*/
		Style knob;

#if LV_USE_ARC
		Style arc_indic;
		Style arc_indic_primary;
#endif

#if LV_USE_BAR
		Style bar;
		Style bar_indic;
#endif

#if LV_USE_BUTTONMATRIX
		Style btnm_bg;
		Style btnm_btn;
		Style btnm_btn_checked;
#endif

#if LV_USE_CHART
		Style chart_series;
		Style chart_indic;
		Style chart_bg;
#endif

#if LV_USE_DROPDOWN
		Style dropdown;
		Style dropdown_list;
#endif

#if LV_USE_CHECKBOX
		Style cb_marker;
		Style cb_marker_checked;
#endif

#if LV_USE_SWITCH
		Style bg_switch;
		Style switch_knob;
#endif

#if LV_USE_LINE
		Style line;
#endif

#if LV_USE_TABLE
		Style table;
		Style table_cell;
#endif

#if LV_USE_TEXTAREA
		Style text_area;
		Style ta_cursor;
		Style ta_placeholder;
#endif

#if LV_USE_CALENDAR
		Style calendar_btnm_bg;
		Style calendar_btnm_day;
		Style calendar_header;
#endif

#if LV_USE_CANVAS
		Style canvas;
#endif

#if LV_USE_MENU
		Style menu_bg;
		Style menu_cont;
		Style menu_sidebar_cont;
		Style menu_main_cont;
		Style menu_page;
		Style menu_header_cont;
		Style menu_header_btn;
		Style menu_section;
		Style menu_pressed;
		Style menu_separator;
#endif

#if LV_USE_MSGBOX
		Style msgbox_backdrop_bg;
#endif

#if LV_USE_KEYBOARD
		Style keyboard_button;
		Style keyboard_button_checked_bg;
#endif

#if LV_USE_LIST
		Style list_bg;
		Style list_btn;
		Style list_item_grow;
#endif

#if LV_USE_TABVIEW
		Style tab_bg_focus;
		Style tab_btn;
#endif
#if LV_USE_LED
		Style led;
#endif

#if LV_USE_SCALE
		Style scale;
#endif

#if LV_USE_SLIDER
		Style slider;
		Style slider_indic;
		Style slider_knob;
#endif
	};

	struct ComponentStyles
	{
		/* Sidebar */
		Style sidebar;				 // Sidebar style
		Style sidebar_btn;			 // Sidebar button style
		Style console_btn_anim;		 // Console button animation style
		Style estop;				 // Emergency stop button style

		/* Files */
		Style file;					 // File item style
		Style folder;				 // Folder item style

		/* Move */
		Style unhomed;				 // Used for the move view

		/* Temperature */
		Style temperature_bar;		 // Temperature bar style
		Style temperature_bar_indic; // Temperature bar indicator style
		Style tool_list;			 // Used in tool list
		Style tool_list_tool;		 // Used in tool list
		Style tool_list_heater;		 // Used in tool list
		Style tool_selected;		 // Used in tool list

		/* Tabview */
		Style tab_button;			 // Used in tabview tabs

		/* Slider */
		Style slider_buttons;		 // Slider increment/decrement buttons

		/* List */
		Style list_header;
		Style list_title;

		/* Graph */
		Style graph_legend;

		/* Bar */
		Style bar_label_bg;
		Style bar_label;

		/* Object cancel */
		Style object_cancel_base;	   // Base style for object cancel list items
		Style object_cancel_current;   // Style for the current object label in the object cancel view
		Style object_cancel_cancelled; // Style for the cancelled object label in the object cancel view

#if DEBUG_BORDERS
		Style debug_borders;
#endif
	};

	const LvglStyles& getLvglStyles();
	const ComponentStyles& getComponentStyles();

	struct FontConfig
	{
		uint32_t size = 14;
		uint32_t style = LV_FREETYPE_FONT_STYLE_NORMAL;
	};

	struct FontConfigSet
	{
		FontConfig header;
		FontConfig normal;
		FontConfig emphasis;
		FontConfig subdued;
	};

	/**
	 * @brief These are the lifetime-managed pointers to the fonts created by the font manager
	 *
	 * @note it is unclear whether
	 */
	struct ThemeFonts
	{
		FontManager::Font header;
		FontManager::Font normal;
		FontManager::Font emphasis;
		FontManager::Font subdued;
	};

	/**
	 * @brief The Theme class copies the font data into a static instance of this struct.
	 *
	 * @note This allows the address of these fonts to be used in `lv_style_set_text_font()` calls. Which means when the
	 * theme is applied or the font is changed, all styles using these fonts will be updated automatically since the
	 * address is the same.
	 */
	struct Fonts
	{
		lv_font_t header;
		lv_font_t normal;
		lv_font_t emphasis;
		lv_font_t subdued;
	};

	const Fonts& getFonts();

	class Theme
	{
	  public:
		Theme(std::string_view name, FontConfigSet fontConfigSet, std::function<void(Theme* theme)> initFunc = nullptr);
		~Theme();
		Theme& operator=(const Theme&) = delete;

		void init();

		void setThemeActive();
		const LvglStyles& getLvglStyles() const;
		const ComponentStyles& getComponentStyles() const;

		const std::string_view getName() const { return m_name; }

		LvglStyles& getLvglStyles();
		ComponentStyles& getComponentStyles();

		void setTypeface(const std::string& typeface);

	  protected:
		virtual void onInit() {}

	  private:
		// Base LVGL styles applied to existing and newly created objects
		std::unique_ptr<LvglStyles> m_lvgl;

		// Specific component styles
		std::unique_ptr<ComponentStyles> m_components;

		const std::string_view m_name;

		/* Fonts */
		FontConfigSet m_fontConfigSet;
		ThemeFonts m_fonts;

		std::function<void(Theme*)> m_initFunc;

		enum class DisplaySize_t
		{
			DISP_SMALL = 3,
			DISP_MEDIUM = 2,
			DISP_LARGE = 1,
		};
		DisplaySize_t m_displaySize;
	};

	void init(lv_display_t* display);
	const std::map<std::string_view, Theme*>& getThemes();
	Theme* getCurrentTheme();
	Theme* getTheme(const size_t index);
	Theme* getThemeByName(std::string_view name);
	Theme* getDefaultTheme();
	size_t getThemeCount();
	bool refreshCurrentTheme();
	const std::vector<std::string_view> getThemeNames();

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

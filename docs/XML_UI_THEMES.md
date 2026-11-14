# Built-in themes

<!-- Generated from the code by tests/src/test_cases/UI/test_xml_ui_api.cpp, like XML_UI_API.md. Don't edit it. -->

The themes the firmware comes with: what each one's file sets, and how it draws. A theme extends another and changes only what makes it different (see [XML_UI.md](XML_UI.md#themes)); the names it uses are in [XML_UI_API.md](XML_UI_API.md#theme). A style's swatch is a box with some text, with the style added; a style that changes nothing on its own (a state's, or a part's) has none.

- [base](#base)
- [3d](#3d)
- [duetscreen](#duetscreen)
- [flat](#flat)
- [grey](#grey)
- [industrial](#industrial)
- [neon](#neon)
- [retro](#retro)
- [soft](#soft)
- [Styles](#styles), in every theme

## base

The root theme; only a base for other themes, so not offered in the settings. Its file is [base.xml](../assets/ui/themes/base.xml).

Palette: `chroma=0.02`, `customisable=true`, `dark=true`, `primary_hue=245`, `secondary_hue=50`

Fonts: emphasis: 14 px, weight 700, bold; header: 18 px, weight 700, bold; huge: 44 px, weight 700, bold; large: 22 px, weight 600, bold; normal: 14 px, weight 400, normal; small: 12 px, weight 400, normal; subdued: 12 px, weight 400, normal; xlarge: 30 px, weight 700, bold

Consts: `#border_width=1`, `#outline_width=2`, `#radius=6`, `#pad_large=10`, `#pad=5`, `#pad_small=2`, `#pad_tiny=1`, `#shadow_offset=3`, `#transition_time=80`, `#widget_regular_min=180`, `#widget_expanded_min=400`, `#widget_gap=5`, `#data_colours=colour`

## 3d

Extends `base`. Its file is [3d.xml](../assets/ui/themes/3d.xml).

Palette: `chroma=0.02`, `dark=false`, `primary_hue=240`, `secondary_hue=70`

Colour adjustments: `primary chroma=0.13`, `primary lightness=0.66 when=light`, `bg_dark lightness=0.9 when=light`, `bg_light lightness=0.97 when=light`

Fonts: emphasis: 14 px, weight 600, bold; normal: 14 px, weight 450, normal

Consts: `#radius=7`

![The palette](../tests/ref_imgs/xml_ui_api/themes/3d/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/3d/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/3d/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/3d/lvgl_widgets.png)

In dark mode:

![The home screen in dark mode](../tests/ref_imgs/xml_ui_api/themes/3d/home_dark.png)

![LVGL's widgets in dark mode](../tests/ref_imgs/xml_ui_api/themes/3d/lvgl_widgets_dark.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/3d/widgets/ds_toggle.png) |

## duetscreen

Extends `base`. Its file is [duetscreen.xml](../assets/ui/themes/duetscreen.xml).

![The palette](../tests/ref_imgs/xml_ui_api/themes/duetscreen/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/duetscreen/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/duetscreen/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/duetscreen/lvgl_widgets.png)

In light mode:

![The home screen in light mode](../tests/ref_imgs/xml_ui_api/themes/duetscreen/home_light.png)

![LVGL's widgets in light mode](../tests/ref_imgs/xml_ui_api/themes/duetscreen/lvgl_widgets_light.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_toggle.png) |

## flat

Extends `base`. Its file is [flat.xml](../assets/ui/themes/flat.xml).

Palette: `chroma=0.01`, `dark=true`, `primary_hue=200`, `secondary_hue=70`

Consts: `#radius=4`

![The palette](../tests/ref_imgs/xml_ui_api/themes/flat/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/flat/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/flat/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/flat/lvgl_widgets.png)

In light mode:

![The home screen in light mode](../tests/ref_imgs/xml_ui_api/themes/flat/home_light.png)

![LVGL's widgets in light mode](../tests/ref_imgs/xml_ui_api/themes/flat/lvgl_widgets_light.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/flat/widgets/ds_toggle.png) |

## grey

Extends `base`. Its file is [grey.xml](../assets/ui/themes/grey.xml).

Palette: `chroma=0`, `customisable=false`, `dark=true`, `primary_hue=245`, `secondary_hue=245`

Colour adjustments: `primary lightness=0.86 chroma=0`, `primary_muted lightness=0.36 chroma=0`, `secondary lightness=0.64 chroma=0`, `secondary_muted lightness=0.42 chroma=0`, `success lightness=0.44 chroma=0`, `warning lightness=0.54 chroma=0`, `series_1 lightness=0.97 chroma=0`, `series_2 lightness=0.6 chroma=0`, `series_3 lightness=0.8 chroma=0`, `series_4 lightness=0.48 chroma=0`, `series_5 lightness=0.89 chroma=0`, `series_6 lightness=0.7 chroma=0`, `series_7 lightness=0.54 chroma=0`, `series_8 lightness=0.75 chroma=0`

Consts: `#data_colours=grey`

![The palette](../tests/ref_imgs/xml_ui_api/themes/grey/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/grey/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/grey/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/grey/lvgl_widgets.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/grey/widgets/ds_toggle.png) |

## industrial

Extends `base`. Its file is [industrial.xml](../assets/ui/themes/industrial.xml).

Palette: `chroma=0.012`, `dark=false`, `primary_hue=240`, `secondary_hue=70`

Colour adjustments: `primary lightness=0.5 chroma=0.13`, `secondary lightness=0.52 chroma=0.12`, `primary_muted lightness=0.88 when=light`, `bg_dark lightness=0.89 when=light`, `bg lightness=0.975 when=light`, `border lightness=0.6 when=light`, `border lightness=0.52 when=dark`

Fonts: emphasis: 14 px, weight 700, bold; header: 18 px, weight 800, bold; normal: 14 px, weight 500, normal; subdued: 12 px, weight 500, normal

Consts: `#radius=2`

![The palette](../tests/ref_imgs/xml_ui_api/themes/industrial/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/industrial/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/industrial/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/industrial/lvgl_widgets.png)

In dark mode:

![The home screen in dark mode](../tests/ref_imgs/xml_ui_api/themes/industrial/home_dark.png)

![LVGL's widgets in dark mode](../tests/ref_imgs/xml_ui_api/themes/industrial/lvgl_widgets_dark.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/industrial/widgets/ds_toggle.png) |

## neon

Extends `base`. Its file is [neon.xml](../assets/ui/themes/neon.xml).

Palette: `chroma=0.03`, `dark=true`, `primary_hue=345`, `secondary_hue=95`

Colour adjustments: `primary chroma=0.19`, `secondary chroma=0.17`, `primary lightness=0.72 when=dark`, `secondary lightness=0.88 when=dark`, `primary lightness=0.56 when=light`, `secondary lightness=0.6 when=light`, `primary_muted from=primary lightness=0.36 chroma=0.12 when=dark`, `secondary_muted from=secondary lightness=0.38 chroma=0.08 when=dark`, `primary_muted from=primary lightness=0.9 chroma=0.06 when=light`, `secondary_muted from=secondary lightness=0.9 chroma=0.06 when=light`, `bg_dark lightness=0.12 when=dark`, `bg lightness=0.165 when=dark`, `bg_light lightness=0.22 when=dark`

![The palette](../tests/ref_imgs/xml_ui_api/themes/neon/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/neon/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/neon/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/neon/lvgl_widgets.png)

In light mode:

![The home screen in light mode](../tests/ref_imgs/xml_ui_api/themes/neon/home_light.png)

![LVGL's widgets in light mode](../tests/ref_imgs/xml_ui_api/themes/neon/lvgl_widgets_light.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/neon/widgets/ds_toggle.png) |

## retro

Extends `base`. Its file is [retro.xml](../assets/ui/themes/retro.xml).

Palette: `chroma=0.035`, `dark=false`, `primary_hue=55`, `secondary_hue=115`

Colour adjustments: `bg_dark lightness=0.9 when=light`, `shadow from=text when=light`, `primary_muted chroma=0.06`

Fonts: emphasis: 15 px, weight 700, bold; header: 20 px, weight 800, bold; normal: 14 px, weight 500, normal; subdued: 12 px, weight 500, normal

Consts: `#border_width=2`, `#radius=0`

![The palette](../tests/ref_imgs/xml_ui_api/themes/retro/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/retro/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/retro/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/retro/lvgl_widgets.png)

In dark mode:

![The home screen in dark mode](../tests/ref_imgs/xml_ui_api/themes/retro/home_dark.png)

![LVGL's widgets in dark mode](../tests/ref_imgs/xml_ui_api/themes/retro/lvgl_widgets_dark.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/retro/widgets/ds_toggle.png) |

## soft

Extends `base`. Its file is [soft.xml](../assets/ui/themes/soft.xml).

Palette: `chroma=0.05`, `dark=false`, `primary_hue=275`, `secondary_hue=170`

Colour adjustments: `bg_dark lightness=0.93 when=light`, `bg lightness=1 when=light`, `bg_light lightness=0.955 when=light`, `primary lightness=0.72 when=light`, `secondary lightness=0.8 when=light`, `bg_dark lightness=0.2 when=dark`, `bg lightness=0.25 when=dark`, `bg_light lightness=0.32 when=dark`

Fonts: emphasis: 14 px, weight 600, bold; header: 18 px, weight 600, bold

Consts: `#radius=12`, `#pad=6`

![The palette](../tests/ref_imgs/xml_ui_api/themes/soft/colours.png)

![The fonts](../tests/ref_imgs/xml_ui_api/themes/soft/fonts.png)

![The home screen](../tests/ref_imgs/xml_ui_api/themes/soft/home.png)

![LVGL's widgets](../tests/ref_imgs/xml_ui_api/themes/soft/lvgl_widgets.png)

In dark mode:

![The home screen in dark mode](../tests/ref_imgs/xml_ui_api/themes/soft/home_dark.png)

![LVGL's widgets in dark mode](../tests/ref_imgs/xml_ui_api/themes/soft/lvgl_widgets_dark.png)

| Widget | Its example |
| --- | --- |
| [`ds_arc`](XML_UI_API.md#ds_arc) | ![ds_arc](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_arc.png) |
| [`ds_bar`](XML_UI_API.md#ds_bar) | ![ds_bar](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_bar.png) |
| [`ds_bar_marker`](XML_UI_API.md#ds_bar_marker) | ![ds_bar_marker](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_bar_marker.png) |
| [`ds_button`](XML_UI_API.md#ds_button) | ![ds_button](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_button.png) |
| [`ds_choice`](XML_UI_API.md#ds_choice) | ![ds_choice](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_choice.png) |
| [`ds_dialog_footer`](XML_UI_API.md#ds_dialog_footer) | ![ds_dialog_footer](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_dialog_footer.png) |
| [`ds_drag_button`](XML_UI_API.md#ds_drag_button) | ![ds_drag_button](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_drag_button.png) |
| [`ds_graph`](XML_UI_API.md#ds_graph) | ![ds_graph](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_graph.png) |
| [`ds_graph_key`](XML_UI_API.md#ds_graph_key) | ![ds_graph_key](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_graph_key.png) |
| [`ds_heatmap`](XML_UI_API.md#ds_heatmap) | ![ds_heatmap](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_heatmap.png) |
| [`ds_hue_slider`](XML_UI_API.md#ds_hue_slider) | ![ds_hue_slider](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_hue_slider.png) |
| [`ds_icon`](XML_UI_API.md#ds_icon) | ![ds_icon](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_icon.png) |
| [`ds_keyboard`](XML_UI_API.md#ds_keyboard) | ![ds_keyboard](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_keyboard.png) |
| [`ds_layout`](XML_UI_API.md#ds_layout) | ![ds_layout](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_layout.png) |
| [`ds_move_handle`](XML_UI_API.md#ds_move_handle) | ![ds_move_handle](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_move_handle.png) |
| [`ds_pages`](XML_UI_API.md#ds_pages) | ![ds_pages](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_pages.png) |
| [`ds_password_toggle`](XML_UI_API.md#ds_password_toggle) | ![ds_password_toggle](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_password_toggle.png) |
| [`ds_plot`](XML_UI_API.md#ds_plot) | ![ds_plot](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_plot.png) |
| [`ds_scope`](XML_UI_API.md#ds_scope) | ![ds_scope](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_scope.png) |
| [`ds_setting`](XML_UI_API.md#ds_setting) | ![ds_setting](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_setting.png) |
| [`ds_slider`](XML_UI_API.md#ds_slider) | ![ds_slider](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_slider.png) |
| [`ds_tabs`](XML_UI_API.md#ds_tabs) | ![ds_tabs](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_tabs.png) |
| [`ds_textarea`](XML_UI_API.md#ds_textarea) | ![ds_textarea](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_textarea.png) |
| [`ds_toggle`](XML_UI_API.md#ds_toggle) | ![ds_toggle](../tests/ref_imgs/xml_ui_api/themes/soft/widgets/ds_toggle.png) |

## Styles

Each style's swatch in each theme. What `base` sets in each is in [XML_UI_API.md](XML_UI_API.md#styles); what a theme changes is in its file.

| Style | 3d | duetscreen | flat | grey | industrial | neon | retro | soft |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `base` |  |  |  |  |  |  |  |  |
| `bg_dark` |  |  |  |  |  |  |  |  |
| `bg` | ![bg](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg.png) | ![bg](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg.png) |
| `bg_light` | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_light.png) | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_light.png) |
| `bg_color_primary` | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_primary.png) | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_primary.png) |
| `bg_color_primary_muted` | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_primary_muted.png) | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_primary_muted.png) |
| `bg_color_secondary` | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_secondary.png) | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_secondary.png) |
| `bg_color_secondary_muted` | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_secondary_muted.png) | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_secondary_muted.png) |
| `bg_color_success` | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_success.png) | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_success.png) |
| `bg_color_warning` | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_warning.png) | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_warning.png) |
| `bg_color_error` | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_error.png) | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_error.png) |
| `bg_color_header` | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_header.png) | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_header.png) |
| `bg_color_list_item` | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_color_list_item.png) | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_color_list_item.png) |
| `text` | ![text](../tests/ref_imgs/xml_ui_api/themes/3d/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/flat/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/grey/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/neon/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/retro/styles/text.png) | ![text](../tests/ref_imgs/xml_ui_api/themes/soft/styles/text.png) |
| `text_muted` | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/3d/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/flat/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/grey/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/neon/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/retro/styles/text_muted.png) | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/soft/styles/text_muted.png) |
| `text_emphasis` | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/3d/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/flat/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/grey/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/neon/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/retro/styles/text_emphasis.png) | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/soft/styles/text_emphasis.png) |
| `text_header` | ![text_header](../tests/ref_imgs/xml_ui_api/themes/3d/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/flat/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/grey/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/neon/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/retro/styles/text_header.png) | ![text_header](../tests/ref_imgs/xml_ui_api/themes/soft/styles/text_header.png) |
| `text_warning` | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/3d/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/flat/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/grey/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/neon/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/retro/styles/text_warning.png) | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/soft/styles/text_warning.png) |
| `border` | ![border](../tests/ref_imgs/xml_ui_api/themes/3d/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/flat/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/grey/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/neon/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/retro/styles/border.png) | ![border](../tests/ref_imgs/xml_ui_api/themes/soft/styles/border.png) |
| `border_muted` | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/3d/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/flat/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/grey/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/neon/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/retro/styles/border_muted.png) | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/soft/styles/border_muted.png) |
| `border_highlight` | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/3d/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/flat/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/grey/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/neon/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/retro/styles/border_highlight.png) | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/soft/styles/border_highlight.png) |
| `border_color_primary` | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/3d/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/flat/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/grey/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/neon/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/retro/styles/border_color_primary.png) | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/soft/styles/border_color_primary.png) |
| `border_color_secondary` | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/3d/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/flat/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/grey/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/neon/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/retro/styles/border_color_secondary.png) | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/soft/styles/border_color_secondary.png) |
| `border_color_card` |  |  |  |  |  |  |  |  |
| `screen` |  |  |  |  |  |  |  |  |
| `scrollbar` | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/3d/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/flat/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/grey/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/neon/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/retro/styles/scrollbar.png) | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/soft/styles/scrollbar.png) |
| `scrollbar_scrolled` | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/3d/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/flat/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/grey/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/neon/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/retro/styles/scrollbar_scrolled.png) | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/soft/styles/scrollbar_scrolled.png) |
| `card` | ![card](../tests/ref_imgs/xml_ui_api/themes/3d/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/flat/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/grey/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/neon/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/retro/styles/card.png) | ![card](../tests/ref_imgs/xml_ui_api/themes/soft/styles/card.png) |
| `tab_card` |  |  |  |  |  |  |  |  |
| `btn` | ![btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/flat/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/btn.png) | ![btn](../tests/ref_imgs/xml_ui_api/themes/soft/styles/btn.png) |
| `btn_checked` | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/3d/styles/btn_checked.png) | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btn_checked.png) |  | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/grey/styles/btn_checked.png) | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/btn_checked.png) | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/neon/styles/btn_checked.png) | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/retro/styles/btn_checked.png) | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/soft/styles/btn_checked.png) |
| `icon` |  |  |  |  |  |  |  |  |
| `icon_recolor` | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/3d/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/flat/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/grey/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/neon/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/retro/styles/icon_recolor.png) | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/soft/styles/icon_recolor.png) |
| `pressed` | ![pressed](../tests/ref_imgs/xml_ui_api/themes/3d/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/flat/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/grey/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/neon/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/retro/styles/pressed.png) | ![pressed](../tests/ref_imgs/xml_ui_api/themes/soft/styles/pressed.png) |
| `disabled` | ![disabled](../tests/ref_imgs/xml_ui_api/themes/3d/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/flat/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/grey/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/neon/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/retro/styles/disabled.png) | ![disabled](../tests/ref_imgs/xml_ui_api/themes/soft/styles/disabled.png) |
| `pad_base` |  |  |  |  |  |  |  |  |
| `pad_zero` |  |  |  |  |  |  |  |  |
| `pad_tiny` |  |  |  |  |  |  |  |  |
| `pad_small` |  |  |  |  |  |  |  |  |
| `pad_normal` |  |  |  |  |  |  |  |  |
| `pad_large` |  |  |  |  |  |  |  |  |
| `pad_gap` |  |  |  |  |  |  |  |  |
| `line_space_large` |  |  |  |  |  |  |  |  |
| `text_align_center` |  |  |  |  |  |  |  |  |
| `outline_primary` | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/3d/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/flat/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/grey/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/neon/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/retro/styles/outline_primary.png) | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/soft/styles/outline_primary.png) |
| `outline_secondary` | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/3d/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/flat/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/grey/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/neon/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/retro/styles/outline_secondary.png) | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/soft/styles/outline_secondary.png) |
| `circle` |  |  |  |  |  |  |  |  |
| `no_radius` |  |  |  |  |  |  |  |  |
| `no_border` |  |  |  |  |  |  |  |  |
| `clip_corner` |  |  |  |  |  |  |  |  |
| `rotary_scroll` |  |  |  |  |  |  |  |  |
| `grow` |  |  |  |  |  |  |  |  |
| `transition_delayed` |  |  |  |  |  |  |  |  |
| `transition_normal` |  |  |  |  |  |  |  |  |
| `anim` |  |  |  |  |  |  |  |  |
| `anim_fast` |  |  |  |  |  |  |  |  |
| `action_btn` | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/flat/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/action_btn.png) | ![action_btn](../tests/ref_imgs/xml_ui_api/themes/soft/styles/action_btn.png) |
| `danger_btn` | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/flat/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/danger_btn.png) | ![danger_btn](../tests/ref_imgs/xml_ui_api/themes/soft/styles/danger_btn.png) |
| `input` | ![input](../tests/ref_imgs/xml_ui_api/themes/3d/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/flat/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/grey/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/neon/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/retro/styles/input.png) | ![input](../tests/ref_imgs/xml_ui_api/themes/soft/styles/input.png) |
| `long_press` |  |  |  |  |  |  |  |  |
| `draggable` |  |  |  |  |  |  |  |  |
| `dragging` | ![dragging](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dragging.png) | ![dragging](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dragging.png) |
| `drag_complete` | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/3d/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/flat/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/grey/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/neon/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/retro/styles/drag_complete.png) | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/soft/styles/drag_complete.png) |
| `knob` | ![knob](../tests/ref_imgs/xml_ui_api/themes/3d/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/flat/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/grey/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/neon/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/retro/styles/knob.png) | ![knob](../tests/ref_imgs/xml_ui_api/themes/soft/styles/knob.png) |
| `arc_indic` |  |  |  |  |  |  |  |  |
| `arc_indic_primary` |  |  |  |  |  |  |  |  |
| `bar` | ![bar](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bar.png) | ![bar](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bar.png) |
| `bar_indic` | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bar_indic.png) | ![bar_indic](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bar_indic.png) |
| `btnm_bg` |  |  |  |  |  |  |  |  |
| `btnm_btn` | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/btnm_btn.png) | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btnm_btn.png) |  | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/btnm_btn.png) | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/btnm_btn.png) | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/btnm_btn.png) | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/btnm_btn.png) | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/soft/styles/btnm_btn.png) |
| `btnm_btn_checked` | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/3d/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/flat/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/grey/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/neon/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/retro/styles/btnm_btn_checked.png) | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/soft/styles/btnm_btn_checked.png) |
| `chart_series` |  |  |  |  |  |  |  |  |
| `chart_indic` | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/3d/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/flat/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/grey/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/neon/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/retro/styles/chart_indic.png) | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/soft/styles/chart_indic.png) |
| `chart_bg` | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/3d/styles/chart_bg.png) | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/chart_bg.png) |  | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/grey/styles/chart_bg.png) | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/chart_bg.png) | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/neon/styles/chart_bg.png) | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/retro/styles/chart_bg.png) |  |
| `dropdown` |  |  |  |  |  |  |  |  |
| `dropdown_list` |  |  |  |  |  |  |  |  |
| `cb_marker` | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/3d/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/flat/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/grey/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/neon/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/retro/styles/cb_marker.png) | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/soft/styles/cb_marker.png) |
| `cb_marker_checked` | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/3d/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/flat/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/grey/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/neon/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/retro/styles/cb_marker_checked.png) | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/soft/styles/cb_marker_checked.png) |
| `bg_switch` | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bg_switch.png) | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bg_switch.png) |
| `switch_knob` | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/3d/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/flat/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/grey/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/neon/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/retro/styles/switch_knob.png) | ![switch_knob](../tests/ref_imgs/xml_ui_api/themes/soft/styles/switch_knob.png) |
| `line` |  |  |  |  |  |  |  |  |
| `table` |  |  |  |  |  |  |  |  |
| `table_cell` | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/3d/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/flat/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/grey/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/neon/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/retro/styles/table_cell.png) | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/soft/styles/table_cell.png) |
| `text_area` |  |  |  |  |  |  |  |  |
| `ta_cursor` | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/3d/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/flat/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/grey/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/neon/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/retro/styles/ta_cursor.png) | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/soft/styles/ta_cursor.png) |
| `ta_placeholder` | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/3d/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/flat/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/grey/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/neon/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/retro/styles/ta_placeholder.png) | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/soft/styles/ta_placeholder.png) |
| `calendar_btnm_bg` |  |  |  |  |  |  |  |  |
| `calendar_btnm_day` | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/3d/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/flat/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/grey/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/neon/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/retro/styles/calendar_btnm_day.png) | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/soft/styles/calendar_btnm_day.png) |
| `calendar_header` |  |  |  |  |  |  |  |  |
| `canvas` | ![canvas](../tests/ref_imgs/xml_ui_api/themes/3d/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/flat/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/grey/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/neon/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/retro/styles/canvas.png) | ![canvas](../tests/ref_imgs/xml_ui_api/themes/soft/styles/canvas.png) |
| `menu_bg` |  |  |  |  |  |  |  |  |
| `menu_cont` |  |  |  |  |  |  |  |  |
| `menu_sidebar_cont` | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/3d/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/flat/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/grey/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/neon/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/retro/styles/menu_sidebar_cont.png) | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/soft/styles/menu_sidebar_cont.png) |
| `menu_main_cont` |  |  |  |  |  |  |  |  |
| `menu_page` |  |  |  |  |  |  |  |  |
| `menu_header_cont` |  |  |  |  |  |  |  |  |
| `menu_header_btn` | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/flat/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/menu_header_btn.png) | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/soft/styles/menu_header_btn.png) |
| `menu_section` | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/3d/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/flat/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/grey/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/neon/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/retro/styles/menu_section.png) | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/soft/styles/menu_section.png) |
| `menu_pressed` | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/3d/styles/menu_pressed.png) | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_pressed.png) | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/flat/styles/menu_pressed.png) | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/grey/styles/menu_pressed.png) | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/menu_pressed.png) | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/neon/styles/menu_pressed.png) | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/retro/styles/menu_pressed.png) |  |
| `menu_separator` |  |  |  |  |  |  |  |  |
| `msgbox_backdrop_bg` |  |  |  |  |  |  |  |  |
| `keyboard_button` | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/3d/styles/keyboard_button.png) | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/keyboard_button.png) |  | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/grey/styles/keyboard_button.png) | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/keyboard_button.png) | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/neon/styles/keyboard_button.png) | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/retro/styles/keyboard_button.png) | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/soft/styles/keyboard_button.png) |
| `keyboard_button_checked_bg` | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/3d/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/flat/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/grey/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/neon/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/retro/styles/keyboard_button_checked_bg.png) | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/soft/styles/keyboard_button_checked_bg.png) |
| `list_bg` |  |  |  |  |  |  |  |  |
| `list_btn` | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/list_btn.png) | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/list_btn.png) |  | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/list_btn.png) | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/list_btn.png) | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/list_btn.png) | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/list_btn.png) |  |
| `list_item_grow` |  |  |  |  |  |  |  |  |
| `tab_bg_focus` |  |  |  |  |  |  |  |  |
| `tab_btn` | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/3d/styles/tab_btn.png) | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/tab_btn.png) |  | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/grey/styles/tab_btn.png) | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/tab_btn.png) | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/neon/styles/tab_btn.png) | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/retro/styles/tab_btn.png) | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/soft/styles/tab_btn.png) |
| `led` | ![led](../tests/ref_imgs/xml_ui_api/themes/3d/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/flat/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/grey/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/neon/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/retro/styles/led.png) | ![led](../tests/ref_imgs/xml_ui_api/themes/soft/styles/led.png) |
| `scale` |  |  |  |  |  |  |  |  |
| `slider` | ![slider](../tests/ref_imgs/xml_ui_api/themes/3d/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/flat/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/grey/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/neon/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/retro/styles/slider.png) | ![slider](../tests/ref_imgs/xml_ui_api/themes/soft/styles/slider.png) |
| `slider_indic` |  |  |  |  |  |  |  |  |
| `slider_knob` | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/3d/styles/slider_knob.png) | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/slider_knob.png) |  | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/grey/styles/slider_knob.png) | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/slider_knob.png) | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/neon/styles/slider_knob.png) | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/retro/styles/slider_knob.png) | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/soft/styles/slider_knob.png) |
| `sidebar` |  |  |  |  |  |  |  |  |
| `sidebar_btn` |  |  |  |  |  |  |  |  |
| `console_btn_anim` |  |  |  |  |  |  |  |  |
| `estop` | ![estop](../tests/ref_imgs/xml_ui_api/themes/3d/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/flat/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/grey/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/neon/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/retro/styles/estop.png) | ![estop](../tests/ref_imgs/xml_ui_api/themes/soft/styles/estop.png) |
| `file` | ![file](../tests/ref_imgs/xml_ui_api/themes/3d/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/flat/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/grey/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/neon/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/retro/styles/file.png) | ![file](../tests/ref_imgs/xml_ui_api/themes/soft/styles/file.png) |
| `folder` | ![folder](../tests/ref_imgs/xml_ui_api/themes/3d/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/flat/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/grey/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/neon/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/retro/styles/folder.png) | ![folder](../tests/ref_imgs/xml_ui_api/themes/soft/styles/folder.png) |
| `folder_icon` | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/3d/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/flat/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/grey/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/neon/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/retro/styles/folder_icon.png) | ![folder_icon](../tests/ref_imgs/xml_ui_api/themes/soft/styles/folder_icon.png) |
| `file_crumb` | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/3d/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/flat/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/grey/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/neon/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/retro/styles/file_crumb.png) | ![file_crumb](../tests/ref_imgs/xml_ui_api/themes/soft/styles/file_crumb.png) |
| `file_crumb_current` |  |  |  |  |  |  |  |  |
| `unhomed` |  |  |  |  |  | ![unhomed](../tests/ref_imgs/xml_ui_api/themes/neon/styles/unhomed.png) |  |  |
| `temperature_bar` | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/3d/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/flat/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/grey/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/neon/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/retro/styles/temperature_bar.png) | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/soft/styles/temperature_bar.png) |
| `temperature_bar_indic` | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/3d/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/flat/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/grey/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/neon/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/retro/styles/temperature_bar_indic.png) | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/soft/styles/temperature_bar_indic.png) |
| `tool_list` |  |  |  |  |  |  |  |  |
| `tool_list_tool` | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/3d/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/flat/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/grey/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/neon/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/retro/styles/tool_list_tool.png) | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/soft/styles/tool_list_tool.png) |
| `tool_list_heater` |  |  |  |  |  |  |  |  |
| `tool_selected` | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/3d/styles/tool_selected.png) | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/tool_selected.png) | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/flat/styles/tool_selected.png) | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/grey/styles/tool_selected.png) | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/tool_selected.png) | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/neon/styles/tool_selected.png) |  | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/soft/styles/tool_selected.png) |
| `heater_standby` |  |  |  |  |  | ![heater_standby](../tests/ref_imgs/xml_ui_api/themes/neon/styles/heater_standby.png) |  |  |
| `heater_off` | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/3d/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/flat/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/grey/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/neon/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/retro/styles/heater_off.png) | ![heater_off](../tests/ref_imgs/xml_ui_api/themes/soft/styles/heater_off.png) |
| `heater_fault` |  |  |  | ![heater_fault](../tests/ref_imgs/xml_ui_api/themes/grey/styles/heater_fault.png) |  | ![heater_fault](../tests/ref_imgs/xml_ui_api/themes/neon/styles/heater_fault.png) |  |  |
| `tab_button` |  |  |  |  |  |  |  |  |
| `slider_buttons` |  |  |  |  |  |  |  |  |
| `list_header` |  |  |  |  |  |  |  |  |
| `list_title` | ![list_title](../tests/ref_imgs/xml_ui_api/themes/3d/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/flat/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/grey/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/neon/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/retro/styles/list_title.png) | ![list_title](../tests/ref_imgs/xml_ui_api/themes/soft/styles/list_title.png) |
| `list_pages` | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/3d/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/flat/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/grey/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/neon/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/retro/styles/list_pages.png) | ![list_pages](../tests/ref_imgs/xml_ui_api/themes/soft/styles/list_pages.png) |
| `list_page_dot` | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/3d/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/flat/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/grey/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/neon/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/retro/styles/list_page_dot.png) | ![list_page_dot](../tests/ref_imgs/xml_ui_api/themes/soft/styles/list_page_dot.png) |
| `list_page_dot_current` |  |  |  |  |  |  |  |  |
| `graph_legend` |  |  |  |  |  |  |  |  |
| `graph_legend_item` | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/3d/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/flat/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/grey/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/neon/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/retro/styles/graph_legend_item.png) | ![graph_legend_item](../tests/ref_imgs/xml_ui_api/themes/soft/styles/graph_legend_item.png) |
| `graph_legend_item_shown` | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/3d/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/flat/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/grey/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/neon/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/retro/styles/graph_legend_item_shown.png) | ![graph_legend_item_shown](../tests/ref_imgs/xml_ui_api/themes/soft/styles/graph_legend_item_shown.png) |
| `graph_legend_swatch` | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/3d/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/flat/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/grey/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/neon/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/retro/styles/graph_legend_swatch.png) | ![graph_legend_swatch](../tests/ref_imgs/xml_ui_api/themes/soft/styles/graph_legend_swatch.png) |
| `graph_legend_swatch_shown` | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/3d/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/flat/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/grey/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/neon/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/retro/styles/graph_legend_swatch_shown.png) | ![graph_legend_swatch_shown](../tests/ref_imgs/xml_ui_api/themes/soft/styles/graph_legend_swatch_shown.png) |
| `graph_legend_value` | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/3d/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/flat/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/grey/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/neon/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/retro/styles/graph_legend_value.png) | ![graph_legend_value](../tests/ref_imgs/xml_ui_api/themes/soft/styles/graph_legend_value.png) |
| `bar_label_bg` | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bar_label_bg.png) | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bar_label_bg.png) |
| `bar_label` | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/3d/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/flat/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/grey/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/neon/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/retro/styles/bar_label.png) | ![bar_label](../tests/ref_imgs/xml_ui_api/themes/soft/styles/bar_label.png) |
| `widget_title` | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/3d/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/flat/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/grey/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/neon/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/retro/styles/widget_title.png) | ![widget_title](../tests/ref_imgs/xml_ui_api/themes/soft/styles/widget_title.png) |
| `widget_value` | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/3d/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/flat/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/grey/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/neon/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/retro/styles/widget_value.png) | ![widget_value](../tests/ref_imgs/xml_ui_api/themes/soft/styles/widget_value.png) |
| `widget_value_large` | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/3d/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/flat/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/grey/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/neon/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/retro/styles/widget_value_large.png) | ![widget_value_large](../tests/ref_imgs/xml_ui_api/themes/soft/styles/widget_value_large.png) |
| `widget_value_huge` | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/3d/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/flat/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/grey/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/neon/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/retro/styles/widget_value_huge.png) | ![widget_value_huge](../tests/ref_imgs/xml_ui_api/themes/soft/styles/widget_value_huge.png) |
| `widget_detail` | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/3d/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/flat/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/grey/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/neon/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/retro/styles/widget_detail.png) | ![widget_detail](../tests/ref_imgs/xml_ui_api/themes/soft/styles/widget_detail.png) |
| `dialog` | ![dialog](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog.png) | ![dialog](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog.png) |  | ![dialog](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog.png) | ![dialog](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog.png) | ![dialog](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog.png) |  | ![dialog](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog.png) |
| `dialog_backdrop` | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_backdrop.png) | ![dialog_backdrop](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_backdrop.png) |
| `dialog_header` | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_header.png) | ![dialog_header](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_header.png) |
| `dialog_title` | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_title.png) | ![dialog_title](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_title.png) |
| `dialog_from` | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_from.png) | ![dialog_from](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_from.png) |
| `dialog_icon_button` |  |  |  |  |  |  |  |  |
| `dialog_body` |  |  |  |  |  |  |  |  |
| `dialog_footer` | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_footer.png) | ![dialog_footer](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_footer.png) |
| `dialog_button` |  |  |  |  |  |  |  |  |
| `dialog_row` | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_row.png) | ![dialog_row](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_row.png) |
| `dialog_row_chosen` | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/3d/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/flat/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/grey/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/neon/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/retro/styles/dialog_row_chosen.png) | ![dialog_row_chosen](../tests/ref_imgs/xml_ui_api/themes/soft/styles/dialog_row_chosen.png) |
| `setting_group` | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/3d/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/flat/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/grey/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/neon/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/retro/styles/setting_group.png) | ![setting_group](../tests/ref_imgs/xml_ui_api/themes/soft/styles/setting_group.png) |
| `setting_group_title` | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/3d/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/flat/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/grey/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/neon/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/retro/styles/setting_group_title.png) | ![setting_group_title](../tests/ref_imgs/xml_ui_api/themes/soft/styles/setting_group_title.png) |
| `setting_group_danger` |  |  |  |  |  |  |  |  |
| `setting_row` | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/3d/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/flat/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/grey/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/neon/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/retro/styles/setting_row.png) | ![setting_row](../tests/ref_imgs/xml_ui_api/themes/soft/styles/setting_row.png) |
| `setting_hint` | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/3d/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/flat/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/grey/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/neon/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/retro/styles/setting_hint.png) | ![setting_hint](../tests/ref_imgs/xml_ui_api/themes/soft/styles/setting_hint.png) |
| `setting_control` |  |  |  |  |  |  |  |  |
| `setting_open` |  |  |  |  |  |  |  |  |
| `numpad` |  |  |  |  |  |  |  |  |
| `numpad_readout` | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/3d/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/flat/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/grey/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/neon/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/retro/styles/numpad_readout.png) | ![numpad_readout](../tests/ref_imgs/xml_ui_api/themes/soft/styles/numpad_readout.png) |
| `numpad_value` | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/3d/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/flat/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/grey/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/neon/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/retro/styles/numpad_value.png) | ![numpad_value](../tests/ref_imgs/xml_ui_api/themes/soft/styles/numpad_value.png) |
| `numpad_key` | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/3d/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/flat/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/grey/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/neon/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/retro/styles/numpad_key.png) | ![numpad_key](../tests/ref_imgs/xml_ui_api/themes/soft/styles/numpad_key.png) |
| `numpad_key_side` |  |  |  |  |  |  |  |  |
| `gauge_arc` |  |  |  |  |  |  |  |  |
| `gauge_arc_indic` |  |  |  |  |  |  |  |  |
| `gauge_arc_target` |  |  |  |  |  |  |  |  |
| `gauge_bar` | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/3d/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/flat/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/grey/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/neon/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/retro/styles/gauge_bar.png) | ![gauge_bar](../tests/ref_imgs/xml_ui_api/themes/soft/styles/gauge_bar.png) |
| `gauge_bar_indic` | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/3d/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/flat/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/grey/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/neon/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/retro/styles/gauge_bar_indic.png) | ![gauge_bar_indic](../tests/ref_imgs/xml_ui_api/themes/soft/styles/gauge_bar_indic.png) |
| `gauge_bar_target` |  |  |  |  |  |  |  |  |
| `gauge_standby` |  |  |  |  |  |  |  |  |
| `gauge_standby_target` |  |  |  |  |  |  |  |  |
| `gauge_off` |  |  |  |  |  |  |  |  |
| `gauge_fault` |  |  |  |  |  |  |  |  |
| `customise_frame` | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/3d/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/flat/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/grey/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/neon/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/retro/styles/customise_frame.png) | ![customise_frame](../tests/ref_imgs/xml_ui_api/themes/soft/styles/customise_frame.png) |
| `customise_frame_selected` | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/3d/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/flat/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/grey/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/neon/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/retro/styles/customise_frame_selected.png) | ![customise_frame_selected](../tests/ref_imgs/xml_ui_api/themes/soft/styles/customise_frame_selected.png) |
| `customise_handle` | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/3d/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/flat/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/grey/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/neon/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/retro/styles/customise_handle.png) | ![customise_handle](../tests/ref_imgs/xml_ui_api/themes/soft/styles/customise_handle.png) |
| `customise_guide` | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/3d/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/flat/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/grey/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/neon/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/retro/styles/customise_guide.png) | ![customise_guide](../tests/ref_imgs/xml_ui_api/themes/soft/styles/customise_guide.png) |
| `customise_place` | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/3d/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/flat/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/grey/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/neon/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/retro/styles/customise_place.png) | ![customise_place](../tests/ref_imgs/xml_ui_api/themes/soft/styles/customise_place.png) |
| `object_cancel_base` | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/3d/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/flat/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/grey/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/neon/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/retro/styles/object_cancel_base.png) | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/soft/styles/object_cancel_base.png) |
| `object_cancel_current` | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/3d/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/flat/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/grey/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/neon/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/retro/styles/object_cancel_current.png) | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/soft/styles/object_cancel_current.png) |
| `object_cancel_cancelled` | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/3d/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/flat/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/grey/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/neon/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/retro/styles/object_cancel_cancelled.png) | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/soft/styles/object_cancel_cancelled.png) |
| `debug_borders` | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/3d/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/flat/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/grey/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/industrial/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/neon/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/retro/styles/debug_borders.png) | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/soft/styles/debug_borders.png) |


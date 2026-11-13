# XML UI API reference

<!-- Generated from the code by tests/src/test_cases/UI/test_xml_ui_api.cpp. Don't edit it: change the code, run the tests (which rewrite this file when it is out of date) and commit the result. -->

Every name the firmware provides to XML, for UI API version 1.0. How to write XML with them is in [XML_UI.md](XML_UI.md). Translation keys, for `translation_tag`, are those in [en-GB.json](../assets/i18n/en-GB.json).

- [Global data](#global-data)
- [Global actions](#global-actions)
- [Presenters](#presenters)
- [Widgets](#widgets)
- [Components](#components)
- [Theme](#theme)

## Global data

Printer data published by the data sources in [src/UI/Xml/Data](../src/UI/Xml/Data). Any view can bind to these. A list is shown with `<ds_list list="…">`; inside each row, the row's own names are found first.

### console

| Name | Type | Description |
| --- | --- | --- |
| `console.lines` | list | What the console shows, oldest first, at most MAX_RESPONSE_LINES |

Each row of `console.lines`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `time` | text | When it arrived, e.g. "[12:34:56]   " |
| `command` | text | A command sent, e.g. "> M115"; empty for a response |
| `text` | text | A response or log message; empty for a command |

### fans

| Name | Type | Description |
| --- | --- | --- |
| `fans` | list | One row per fan, in slot order |

Each row of `fans`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `number` | int | Fan number, as used by M106 P |
| `name` | text | Display name, e.g. "Fan 1" |
| `requested` | int | Requested speed, percent 0-100 |
| `actual` | int | Measured speed, percent 0-100 |
| `rpm` | int | Tachometer reading, -1 if none |
| `thermostatic` | int | 1 if the fan is temperature controlled, else 0 |

### tools

| Name | Type | Description |
| --- | --- | --- |
| `tools` | list | One row per tool, in slot order |

Each row of `tools`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `number` | int | Tool number, as used by T |
| `name` | text | Display name |
| `selected` | int | 1 if this is the active tool, else 0 |
| `heater_count` | int | Number of heaters |
| `heaters` | list | One row per heater of the tool |

Each row of `tools` › `heaters`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `heater` | int | Heater number |
| `name` | text | Display name, e.g. "Heater 1" or "Bed" |
| `state` | text | Status key: active, standby, off, fault, tuning, offline or unknown |
| `status` | text | Status, translated for display |
| `current` | text | Current temperature as text, one decimal place, e.g. "21.2" |
| `current_value` | int | Current temperature, whole °C, for a bar |
| `scale_min` | int | Bottom of a temperature scale, °C: the heater's minimum, or 0 if that is below 0 |
| `scale_max` | int | Top of a temperature scale, °C: the heater's maximum |
| `active` | int | Active target temperature, °C |
| `standby` | int | Standby target temperature, °C |

### heat

| Name | Type | Description |
| --- | --- | --- |
| `beds` | list | One row per bed, in slot order |
| `chambers` | list | One row per chamber, in slot order |

Each row of `beds`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `number` | int | Bed or chamber number, as used by M140/M141 P |
| `heater` | int | Heater number |
| `name` | text | Display name, e.g. "Heater 1" or "Bed" |
| `state` | text | Status key: active, standby, off, fault, tuning, offline or unknown |
| `status` | text | Status, translated for display |
| `current` | text | Current temperature as text, one decimal place, e.g. "21.2" |
| `current_value` | int | Current temperature, whole °C, for a bar |
| `scale_min` | int | Bottom of a temperature scale, °C: the heater's minimum, or 0 if that is below 0 |
| `scale_max` | int | Top of a temperature scale, °C: the heater's maximum |
| `active` | int | Active target temperature, °C |
| `standby` | int | Standby target temperature, °C |

Each row of `chambers`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `number` | int | Bed or chamber number, as used by M140/M141 P |
| `heater` | int | Heater number |
| `name` | text | Display name, e.g. "Heater 1" or "Bed" |
| `state` | text | Status key: active, standby, off, fault, tuning, offline or unknown |
| `status` | text | Status, translated for display |
| `current` | text | Current temperature as text, one decimal place, e.g. "21.2" |
| `current_value` | int | Current temperature, whole °C, for a bar |
| `scale_min` | int | Bottom of a temperature scale, °C: the heater's minimum, or 0 if that is below 0 |
| `scale_max` | int | Top of a temperature scale, °C: the heater's maximum |
| `active` | int | Active target temperature, °C |
| `standby` | int | Standby target temperature, °C |

### job

| Name | Type | Description |
| --- | --- | --- |
| `job.file` | text | The current job's file, e.g. "0:/gcodes/part.gcode"; empty when there is none |

### machine

| Name | Type | Description |
| --- | --- | --- |
| `machine.status` | text | State key, e.g. idle, processing, paused; translated under state.* |
| `machine.printing` | int | 1 while a job is running and not paused, else 0: controls that would change the job are disabled |
| `machine.job_active` | int | 1 while there is a job (running, pausing, paused, resuming or simulating), else 0 |

### navigation

| Name | Type | Description |
| --- | --- | --- |
| `nav.screen` | text | Name of the open screen, or empty when the home screens are showing |

### responses

| Name | Type | Description |
| --- | --- | --- |
| `responses.count` | int | Number of responses received; changes with every response |

### temperatures

| Name | Type | Description |
| --- | --- | --- |
| `temperatures` | history | One series per heater, in slot order, sampled every model tick, whole °C. Heaters sharing a sensor are shown once. |
| `temperatures.limit` | int | Highest maximum temperature of any heater, °C (0 with no heaters), e.g. for a graph's axis |

Each row of `temperatures`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `name` | text | Name of the series, e.g. for a legend |

## Global actions

Actions any view can raise with `<action name="…">`, whatever its presenter. A presenter's own action of the same name is found first.

| Action | Description |
| --- | --- |
| `gcode` | Send `arg` to the printer as G-code, e.g. `arg="M106 P{number} S{value}"` |
| `heaters.set_active` | Set the heater's active temperature to the edited value (`value_confirmed`), else ask for it with a number pad. From the same rows as `heaters.toggle`. |
| `heaters.set_standby` | As `heaters.set_active`, for the standby temperature |
| `heaters.toggle` | Cycle the heater through active, standby and off. From a tool's `heaters` row, or a row of `beds` or `chambers`. |
| `nav.back` | Close the top modal, or else go back a screen |
| `nav.home` | Close every screen and show the home screens |
| `nav.open` | Open the screen registered as `arg`, e.g. `arg="control"` |
| `tools.toggle` | Select the tool, or deselect it if selected. From a row of `tools`. |

An action runs when its widget raises its `trigger` (default `clicked`): `all`, `pressed`, `pressing`, `press_lost`, `short_clicked`, `single_clicked`, `double_clicked`, `triple_clicked`, `long_pressed`, `long_pressed_repeat`, `clicked`, `released`, `scroll_begin`, `scroll_throw_begin`, `scroll_end`, `scroll`, `gesture`, `key`, `rotary`, `focused`, `defocused`, `leave`, `hit_test`, `indev_reset`, `hover_over`, `hover_leave`, `cover_check`, `refr_ext_draw_size`, `draw_main_begin`, `draw_main`, `draw_main_end`, `draw_post_begin`, `draw_post`, `draw_post_end`, `draw_task_added`, `value_changed`, `insert`, `refresh`, `ready`, `cancel`, `state_changed`, `create`, `delete`, `child_changed`, `child_created`, `child_deleted`, `screen_unload_start`, `screen_load_start`, `screen_loaded`, `screen_unloaded`, `size_changed`, `style_changed`, `layout_changed`, `get_self_size`, `invalidate_area`, `resolution_changed`, `color_format_changed`, `refr_request`, `refr_start`, `refr_ready`, `render_start`, `render_ready`, `flush_start`, `flush_finish`, `flush_wait_start`, `flush_wait_finish`, `vsync`, `value_confirmed`, `dismissed`

## Presenters

A component names its presenter with `<component presenter="…">`. Its view can then bind to the presenter's names and raise its actions, as well as the global ones.

### babystep

Used by `babystep`.

| Name | Type | Description |
| --- | --- | --- |
| `usable` | int | 1 if there is a Z axis and the printer is connected |
| `reset_text` | text | The reset button's text, with the current offset, e.g. "Reset 0.02 mm" |
| `steps` | list | Step sizes |

Each row of `steps`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | E.g. "0.01 mm" |
| `selected` | int | 1 for the step in use |

| Action | Description |
| --- | --- |
| `babystep.down` | Move Z down by the chosen step (M290) |
| `babystep.edit` | Ask for a new size for that step with a number pad, and remember it. From a row of `steps`. |
| `babystep.reset` | Undo all babystepping |
| `babystep.select` | Use that step. From a row of `steps`. |
| `babystep.up` | Move Z up by the chosen step (M290) |

### console

Used by `console`.

| Name | Type | Description |
| --- | --- | --- |
| `input` | text | What is typed, kept equal to the text box so picking a G-code can replace it |
| `commands_shown` | int | 1 when the G-code list is showing, 0 when it is folded away; remembered |
| `shell_available` | int | 1 if this build can run shell commands (ENABLE_CONSOLE_SHELL, e.g. developer builds) |
| `shell` | int | 1 while what is sent goes to the shell instead of the Duet |
| `gcodes` | list | The G-codes starting with the first word typed |

Each row of `gcodes`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `gcode` | text | The G-code, e.g. "G28" |
| `description` | text | What it does, e.g. "Home" |

| Action | Description |
| --- | --- |
| `console.clear` | Empty the text box |
| `console.input` | What is typed changed to the text box's text (`value_changed` on it): narrow the G-code list to match |
| `console.pick` | Put the G-code in the text box. From a row of `gcodes`. |
| `console.send` | Send what is typed: to the shell while `shell` is 1, else to the printer as G-code |
| `console.shell` | Send to the shell while the edited value is non-zero (`value_changed` on a checkbox), if `shell_available` |
| `console.toggle_commands` | Show or fold away the G-code list, and remember which |

### dashboard_jobs, dashboard_macros, files_jobs, files_macros

| Name | Type | Description |
| --- | --- | --- |
| `files` | list | One row per file and folder in the open folder, in the chosen order |
| `crumbs` | list | The open folder's path from the base folder down, e.g. "0:/gcodes", "parts" |
| `sort` | int | Order: 0 by name, 1 by date, 2 by size |
| `sort_name` | text | The name-order button's text, with an arrow showing the direction when it is in use |
| `sort_date` | text | As `sort_name`, for the date order |
| `sort_size` | text | As `sort_name`, for the size order |
| `tiles` | int | 1 when files are shown as tiles, 0 as a list |
| `mode_icon` | text | Symbol for the other way of showing files |
| `dialog` | text | What the dialog asks: "start_print", "run_macro" or "delete"; empty when it is closed |
| `dialog_title` | text | Dialog title |
| `dialog_text` | text | Dialog message |
| `dialog_image` | text | Thumbnail shown in the dialog, or empty |
| `dialog_has_image` | int | 1 if there is a thumbnail, else 0 |
| `dialog_can_delete` | int | 1 if the dialog offers to delete the file (starting a job does), else 0 |
| `info_shown` | int | 1 if the job's details (the info_* fields) are known, else 0 |
| `info_date` | text | Job modification date |
| `info_size` | text | Job size |
| `info_generated_by` | text | Slicer |
| `info_print_time` | text | Estimated print time |
| `info_height` | text | Object height |
| `info_layer_height` | text | Layer height |

Each row of `files`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `name` | text | Shown name; a macro's "12_" ordering prefix is left out |
| `date` | text | Modification date, e.g. "2025-03-18 18:21:58" |
| `thumbnail` | text | Thumbnail image path, or empty if there is none (yet) |
| `is_folder` | int | 1 for a folder, else 0 |

Each row of `crumbs`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `name` | text | The folder's name |
| `last` | int | 1 for the open folder, else 0 (the others can be tapped to go back up) |

| Action | Description |
| --- | --- |
| `files.ask_delete` | Ask to delete the file the dialog is about |
| `files.close_dialog` | Close the dialog without doing anything |
| `files.confirm` | Do what the dialog asks (start the job, run the macro or delete the file), and close it |
| `files.crumb` | Open that folder. From a row of `crumbs`. |
| `files.open` | Open the folder, or ask to start the job or run the macro. From a row of `files`. |
| `files.refresh` | Close the dialog and read the folder again |
| `files.sort` | Sort by `arg`: `name`, `date` or `size`; choosing the order in use reverses it. Remembered. |
| `files.toggle_mode` | Show tiles instead of a list, or the other way round. Remembered. |

### extruder

Used by `extruder`.

| Name | Type | Description |
| --- | --- | --- |
| `extrude_usable` | int | 1 if the selected tool can extrude: there is one, it is hot enough, and no job is running |
| `retract_usable` | int | 1 if the selected tool can retract: there is one, it is hot enough, and no job is running |
| `distance.title` | text | The title, e.g. "Feed Distance (mm)" |
| `distance.value` | text | The value, e.g. "0.1" |
| `distance.choices` | list | The presets, each with `text` |
| `feedrate.title` | text | The title, e.g. "Feed Distance (mm)" |
| `feedrate.value` | text | The value, e.g. "0.1" |
| `feedrate.choices` | list | The presets, each with `text` |

Each row of `distance.choices`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | The preset, e.g. "10" |

Each row of `feedrate.choices`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | The preset, e.g. "10" |

| Action | Description |
| --- | --- |
| `extruder.extrude` | Extrude the chosen distance at the chosen feed rate with the selected tool |
| `extruder.retract` | Retract the chosen distance at the chosen feed rate with the selected tool |
| `selector.decrement` | Step the selector named by `arg` (its id) down |
| `selector.edit` | Ask for a value for the selector named by `arg` (its id) with a number pad |
| `selector.edit_choice` | Ask for a new value for the preset with a number pad, and use it. From a row of a selector's `choices`. |
| `selector.increment` | Step the selector named by `arg` (its id) up |
| `selector.pick` | Use the preset. From a row of a selector's `choices`. |

### fans

Used by `fan_screen`.

| Action | Description |
| --- | --- |
| `fans.set_speed` | Set the fan's speed in percent: the edited value (`value_confirmed`), else `arg`. From a row of `fans`. |

### heightmap

Used by `heightmap`.

| Name | Type | Description |
| --- | --- | --- |
| `surface` | surface | The height map shown, sampled for <ds_heatmap> |
| `title` | text | E.g. "Heightmap: heightmap.csv" |
| `maps` | list | The height map files |
| `stats.points` | text | Number of points, as text, e.g. "Number of points: 0;" |
| `stats.area` | text | Probed area, as text |
| `stats.min` | text | Lowest deviation, as text |
| `stats.max` | text | Highest deviation, as text |
| `stats.mean` | text | Mean error, as text |
| `stats.rms` | text | RMS error, as text |
| `mode` | int | How values are coloured: 0 fixed (±0.25 mm), 1 auto (the map's own range) |
| `levelling_usable` | int | 1 if the bed can be levelled or probed: connected and no job running |

Each row of `maps`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `name` | text | File name |
| `loaded` | int | 1 for the one the printer is using |
| `load_text` | text | "Load", or "Unload" for the one in use |

| Action | Description |
| --- | --- |
| `heightmap.mode` | Colour values by `arg`: `fixed` (±0.25 mm) or `auto` (the map's own range) |
| `heightmap.show` | Show that height map. From a row of `maps`. |
| `heightmap.toggle` | Make the printer use that height map, or stop using it, and show it. From a row of `maps`. |

### job_status

Used by `job_status`.

| Name | Type | Description |
| --- | --- | --- |
| `file` | text | The current job's file, else the last job's |
| `finished` | int | 1 when showing the last job (none is running), else 0 |
| `progress` | int | Progress, percent 0-100 |
| `thumbnail` | text | The job's thumbnail path, or empty until it arrives |
| `axes` | list | One row per axis |
| `speed_factor` | text | Speed override, e.g. "100%" |
| `speed_text` | text | Current and requested speed, e.g. "0/0 mm/s" |
| `speed_top` | int | The current move's top speed, in thousandths of the fastest axis' maximum (0-1000) |
| `speed_requested` | int | Requested speed, in thousandths of the fastest axis' maximum (0-1000) |
| `flow_factor` | text | Extrusion override, e.g. "100%" |
| `flow_text` | text | Volumetric flow, e.g. "0.0 mm³/s" |
| `flow` | int | Volumetric flow, in thousandths of the fastest extruder's maximum (0-1000) |
| `babystep` | text | Z babystep, e.g. "0 mm" |
| `babystep_open` | int | 1 while the babystepping dialog is open |
| `elapsed` | text | Elapsed time, e.g. "Elapsed: 00:03:43" |
| `remaining` | text | Time left, e.g. "Remaining: 00:45:03" |
| `pause` | int | Pause: 0 hidden, 1 disabled, 2 usable |
| `resume` | int | Resume: 0 hidden, 1 disabled, 2 usable |
| `print_again` | int | Print again: 0 hidden, 1 disabled, 2 usable |
| `cancel` | int | Cancel: 0 hidden, 1 disabled, 2 usable |
| `confirm_cancel` | int | 1 while asking whether to cancel the job |

Each row of `axes`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | Letter and user position on two lines, e.g. "X\n0.00" |

| Action | Description |
| --- | --- |
| `job.ask_cancel` | Ask whether to cancel the job (sets `confirm_cancel`) |
| `job.babystep` | Open the babystepping dialog (sets `babystep_open`) |
| `job.cancel` | Cancel the job, which also turns the heaters off, and close the question |
| `job.close_babystep` | Close the babystepping dialog |
| `job.close_dialog` | Close the question without cancelling the job |
| `job.flow_factor` | Open the dialog to change the extrusion override |
| `job.pause` | Pause the job |
| `job.print_again` | Start the last job again |
| `job.resume` | Resume the paused job |
| `job.speed_factor` | Open the dialog to change the speed override |

### move

Used by `move`.

| Name | Type | Description |
| --- | --- | --- |
| `connected` | int | 1 while connected |
| `x.label` | text | Position in the displayed unit, e.g. "X: 0.00" |
| `x.homed` | int | 1 when homed |
| `x.jog_usable` | int | 1 if the axis can be jogged or set (homed, or moves before homing allowed) |
| `x.home_usable` | int | 1 if it can be homed |
| `y.label` | text | Position in the displayed unit, e.g. "X: 0.00" |
| `y.homed` | int | 1 when homed |
| `y.jog_usable` | int | 1 if the axis can be jogged or set (homed, or moves before homing allowed) |
| `y.home_usable` | int | 1 if it can be homed |
| `z.label` | text | Position in the displayed unit, e.g. "X: 0.00" |
| `z.homed` | int | 1 when homed |
| `z.jog_usable` | int | 1 if the axis can be jogged or set (homed, or moves before homing allowed) |
| `z.home_usable` | int | 1 if it can be homed |
| `xy.homed` | int | 1 when X and Y are homed |
| `xy.home_usable` | int | 1 if X and Y can be homed |
| `home_all_usable` | int | 1 if all axes can be homed |
| `disable_motors_usable` | int | 1 if the motors can be disabled |
| `axes` | list | The axes other than X, Y and Z |
| `axes_shown` | int | 1 if there are other axes |
| `distance_title` | text | Distance row title, e.g. "Move Distance (mm)" |
| `feedrate_title` | text | Speed row title |
| `distances` | list | The jog distances to choose from |
| `feedrates` | list | The jog speeds to choose from |
| `confirm` | int | 1 while asking before homing or disabling the motors |
| `confirm_title` | text | The question's title |
| `confirm_text` | text | The question |

Each row of `axes`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `letter` | text | Axis letter |
| `label` | text | Position in the displayed unit, e.g. "U: 0.00" |
| `homed` | int | 1 when homed |
| `jog_usable` | int | 1 if the axis can be jogged or set (homed, or moves before homing allowed) |
| `home_usable` | int | 1 if it can be homed |

Each row of `distances`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | The value |
| `selected` | int | 1 for the one in use |

Each row of `feedrates`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | The value |
| `selected` | int | 1 for the one in use |

| Action | Description |
| --- | --- |
| `move.close_dialog` | Answer no to the question (`confirm`): close it without doing anything |
| `move.confirm` | Answer yes to the question (`confirm`): close it and do what it asked |
| `move.disable_motors` | Disable the motors (M18), asking first if confirmations are on |
| `move.distance` | Use that jog distance. From a row of `distances`. |
| `move.edit_distance` | Ask for a new value for that jog distance with a number pad. From a row of `distances`. |
| `move.edit_feedrate` | Ask for a new value for that jog speed with a number pad. From a row of `feedrates`. |
| `move.feedrate` | Use that jog speed. From a row of `feedrates`. |
| `move.home` | Home: `arg` is `X`, `Y`, `Z`, `XY` or `all`, or raise it from a row of `axes`. Asks first if confirmations are on. |
| `move.jog` | Jog by the chosen distance at the chosen speed. `arg` is `X+`, `Z-`, …; from a row of `axes`, `+` or `-`. |
| `move.set_position` | Ask for a position and move there. `arg` is the axis letter, or raise it from a row of `axes`. |

### notifications

Used by `notifications`.

| Name | Type | Description |
| --- | --- | --- |
| `response.open` | int | 1 while a response is showing |
| `response.title` | text | "Info", "Success", "Warning" or "Error" |
| `response.text` | text | The response |
| `response.type` | int | 0 info, 1 success, 2 warning, 3 error |
| `response.progress` | int | Time left before it closes itself, 0-100 |
| `update.open` | int | 1 while an update is offered or its result shown |
| `update.title` | text | Title |
| `update.text` | text | Text |
| `update.ok_text` | text | OK button text |
| `update.can_cancel` | int | 1 if the update can be declined |
| `alert.open` | int | 1 while an M291 alert is showing |
| `alert.title` | text | Title |
| `alert.text` | text | Text |
| `alert.ok` | int | The OK button: 0 hidden, 1 disabled (the input isn't valid), 2 usable |
| `alert.ok_text` | text | "Ok", or "Close" for a message that only needs closing |
| `alert.cancel` | int | The Cancel button: 0 hidden, 2 usable |
| `alert.footer_shown` | int | 1 if there is an OK or Cancel button |
| `alert.choices` | list | The choices offered, each with `text` |
| `alert.choices_shown` | int | 1 if choices are offered |
| `alert.axes` | list | The axes the user may jog, each with `label` and `homed` |
| `alert.axes_shown` | int | 1 if there are axes to jog |
| `alert.input_shown` | int | 1 if a value is asked for |
| `alert.input` | text | The value to start with |
| `alert.accepted_chars` | text | Characters the value may contain; empty for any |
| `alert.keyboard_mode` | text | "number" or "text" |
| `alert.min` | text | Lower limit shown beside the value; empty if none |
| `alert.max` | text | Upper limit shown beside the value; empty if none |
| `alert.warning` | text | Why the value isn't valid; empty if it is |
| `alert.keyboard` | int | 1 while the keyboard is up, so the alert moves out of its way |
| `alert.progress` | int | Time left before the alert closes itself, 0-100 |
| `alert.progress_shown` | int | 1 if the alert closes itself |

Each row of `alert.choices`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `text` | text | The choice |

Each row of `alert.axes`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `label` | text | Axis and position, e.g. "X = 0.00" |
| `homed` | int | 1 if the axis can be jogged |

| Action | Description |
| --- | --- |
| `alert.cancel` | Answer the alert with Cancel (M292 P1) if it can be cancelled, and close it |
| `alert.choose` | Answer the alert with the choice, and close it. From a row of `alert.choices`. |
| `alert.dismissed` | Close the alert without an answer, e.g. on `dismissed` from its `<ds_modal>` |
| `alert.input` | Check the text edited (`value_changed` on the value's text box), setting `alert.warning` and `alert.ok` |
| `alert.jog` | Move the axis by the edited value, or else `arg`, in mm. From a row of `alert.axes`. |
| `alert.keyboard` | Set `alert.keyboard` to `arg`, 1 when the keyboard comes up and 0 when it goes away |
| `alert.ok` | Answer the alert with OK (M292), with the value typed if one was asked for, and close it |
| `response.close` | Close the response showing and show the next |
| `response.open_console` | Forget every response and open the console |
| `update.close` | Close the update prompt without installing |
| `update.confirm` | Install the update offered (nothing when a result is showing), and close |

### object_cancel

Used by `object_cancel`.

| Name | Type | Description |
| --- | --- | --- |
| `current_object` | text | Name of the object being printed, or "None" |
| `x_min` | int | Lowest X of the bed, whole mm: the X axis' travel allowing for the tool's offset, or the objects' bounds with a margin if the axes aren't known |
| `x_max` | int | Highest X of the bed, whole mm; as `x_min` |
| `y_min` | int | Lowest Y of the bed, whole mm; as `x_min` |
| `y_max` | int | Highest Y of the bed, whole mm; as `x_min` |
| `objects` | list | The job's objects |
| `confirmations` | int | 1 if actions are confirmed first (the setting) |
| `confirm` | int | 1 while asking; `confirm_title` and `confirm_text` are the question |
| `confirm_title` | text | Question title |
| `confirm_text` | text | Question |

Each row of `objects`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `number` | int | Object number |
| `label` | text | E.g. "[2]: gear (Instance 37)" |
| `x1` | int | Lowest X of its bounds, whole mm |
| `y1` | int | Lowest Y of its bounds, whole mm |
| `x2` | int | Highest X of its bounds, whole mm |
| `y2` | int | Highest Y of its bounds, whole mm |
| `current` | int | 1 if it is being printed |
| `cancelled` | int | 1 if it has been cancelled |

| Action | Description |
| --- | --- |
| `object_cancel.cancel_current` | Cancel the object being printed, asking first if confirmations are on |
| `object_cancel.close_dialog` | Answer no to the question (`confirm`): close it without doing anything |
| `object_cancel.confirm` | Answer yes to the question (`confirm`): close it and do what it asked |
| `object_cancel.select` | Cancel the object, or resume it if cancelled, asking first. From a row of `objects`. |

### settings_connection

Used by `settings_connection`.

| Name | Type | Description |
| --- | --- | --- |
| `connection_methods` | text | Connection methods offered (USB, UART or network), one per line |
| `connection_method` | int | Index of the one in use, in `connection_methods` |
| `usb_modes` | text | Modes of the USB-C port offered, one per line |
| `usb_mode` | int | Index of the one in use, in `usb_modes` |
| `poll_interval` | int | ms between polls of the Duet |
| `network_shown` | int | 1 if the Duet is reached over the network, so its IP address and password are shown |
| `duet_ip` | text | The Duet's IP address |
| `duet_password` | text | The Duet's password |

| Action | Description |
| --- | --- |
| `settings.connection_method` | Connect to the Duet by the method at the edited index of `connection_methods`, setting the USB-C mode to suit |
| `settings.duet_ip` | Ask for the Duet's IP address with the nearest number pad, and use it |
| `settings.duet_password` | Use the text edited (e.g. a `<ds_textarea>`'s) as the Duet's password |
| `settings.poll_interval` | Poll the Duet every edited value ms |
| `settings.usb_mode` | Use the USB-C port in the mode at the edited index of `usb_modes` |

### settings_developer

Used by `settings_developer`.

| Name | Type | Description |
| --- | --- | --- |
| `debug_levels` | text | Log levels offered, one per line |
| `debug_level` | int | Index of the one in use, in `debug_levels` |
| `advanced_settings` | int | 1 if advanced settings are shown |
| `second_usb_channel` | int | 1 if the second USB channel is used |
| `system_logging` | int | 1 if system messages are logged to the console |
| `debug_borders_shown` | int | 1 if the build has debug borders |
| `debug_borders` | int | 1 if debug borders are on (builds with debug borders) |
| `developer_mode` | int | 1 in developer mode (SSH, ADB, setup) |
| `ssh` | int | 1 if SSH is on (developer mode builds) |
| `adb` | int | 1 if ADB is on (developer mode builds) |
| `system_monitor_shown` | int | 1 if the build has the system monitor |
| `system_monitor` | int | 1 if the system monitor is on |
| `storage_info` | text | Storage used, e.g. "256.00 MB / 16.00 GB (1%)" |
| `cache_size` | text | Size of the cache, e.g. "10.00 KB (100 files)" |

| Action | Description |
| --- | --- |
| `settings.adb` | Turn ADB on (edited value 1) or off (0) (developer mode builds) |
| `settings.advanced_settings` | Show advanced settings if the edited value is 1, else hide them |
| `settings.buildroot_setup` | Restart the system's setup service (developer mode builds) |
| `settings.clear_cache` | Delete the cached settings and file information, then show the storage used again |
| `settings.debug_borders` | Show debug borders if the edited value is 1, else hide them (builds with debug borders) |
| `settings.debug_level` | Log at the level at the edited index of `debug_levels` |
| `settings.erase_and_restart` | Erase every setting, then restart |
| `settings.hardware_test` | Open the hardware test |
| `settings.reboot` | Reboot the screen |
| `settings.restart` | Restart the DuetScreen program |
| `settings.second_usb_channel` | Use the second USB channel if the edited value is 1, else not |
| `settings.send_config` | Upload the screen's settings to the Duet as duetscreen.json in its system directory |
| `settings.ssh` | Turn SSH on (edited value 1) or off (0) (developer mode builds) |
| `settings.system_logging` | Log system messages to the console if the edited value is 1, else not |
| `settings.system_monitor` | Show the system monitor if the edited value is 1, else hide it |
| `settings.upgrade_from_github` | Upgrade to the latest release on GitHub, reporting an error if it fails to start |

### settings_display

Used by `settings_display`.

| Name | Type | Description |
| --- | --- | --- |
| `themes` | text | Themes offered, one per line |
| `theme` | int | Index of the theme in use |
| `fonts` | text | Typefaces offered, one per line |
| `font` | int | Index of the typeface in use |
| `icon_sets` | text | Icon sets offered, one per line |
| `icon_set` | int | Index of the icon set in use |
| `rotations` | text | Screen rotations offered, one per line |
| `rotation` | int | Index of the rotation in use |
| `animations` | int | 1 if the UI animates |

| Action | Description |
| --- | --- |
| `settings.animations` | Turn UI animations on (edited value 1) or off (0) |
| `settings.font` | Use the typeface at the edited index of `fonts` |
| `settings.icon_set` | Use the icon set at the edited index of `icon_sets` |
| `settings.rotation` | Rotate the screen as the edited index of `rotations` says |
| `settings.theme` | Use the theme at the edited index of `themes` |

### settings_general

Used by `settings_general`.

| Name | Type | Description |
| --- | --- | --- |
| `firmware` | text | Firmware version and build time |
| `buildroot` | text | Buildroot version |
| `languages` | text | Languages offered, one per line |
| `language` | int | Index of the language in use, in `languages` |
| `keyboard_layouts` | text | USB keyboard layouts, one per line |
| `keyboard_layout` | int | Index of the layout in use, in `keyboard_layouts` |
| `brightness` | int | Brightness, 0-100 |
| `screensaver_timeout` | int | Screensaver timeout, s |
| `confirmations` | int | 1 if actions are confirmed first |
| `machine_position_mode` | int | 1 if absolute moves use machine coordinates (G53) |
| `progress_sources` | text | How a job's progress can be worked out, one per line |
| `progress_source` | int | Index of the one used, in `progress_sources` |
| `connected_message` | int | 1 if a message is shown on connecting |
| `notification_levels` | text | Notification levels, one per line |
| `notification_level` | int | Index of the lowest level shown, in `notification_levels` |
| `notification_timeout` | int | ms before a notification closes itself |
| `error_notifications_stay` | int | 1 if error notifications stay until closed |
| `language_confirm` | int | 1 while asking to restart in another language |
| `language_confirm_title` | text | The language question's title |
| `language_confirm_text` | text | The language question |

| Action | Description |
| --- | --- |
| `settings.brightness` | Set the screen brightness to the edited value, 0-100 |
| `settings.confirmations` | Turn confirmation dialogs on (edited value 1) or off (0) |
| `settings.connected_message` | Show a message on connecting if the edited value is 1, else not |
| `settings.error_notifications_stay` | Keep error notifications open until closed if the edited value is 1, else not |
| `settings.keep_language` | Answer no to `language_confirm`: keep the language in use |
| `settings.keyboard_layout` | Use the USB keyboard layout at the edited index of `keyboard_layouts` |
| `settings.language` | Ask whether to restart in the language at the edited index of `languages` |
| `settings.machine_position_mode` | Use machine coordinates (G53) for absolute moves if the edited value is 1, else not |
| `settings.notification_level` | Show notifications from the level at the edited index of `notification_levels` up |
| `settings.notification_timeout` | Close notifications after the edited value, in ms |
| `settings.progress_source` | Work out job progress with the source at the edited index of `progress_sources` |
| `settings.restart_in_language` | Answer yes to `language_confirm`: store the language picked and restart |
| `settings.screensaver_timeout` | Set the screensaver timeout to the edited value, in seconds |

### status_bar

Used by `status_bar`.

| Name | Type | Description |
| --- | --- | --- |
| `status` | text | Connection and state, e.g. "USB - Idle" |
| `name` | text | The printer's name |
| `has_tool` | int | 1 if there is a current tool with heaters, else 0 |
| `tool` | text | The current tool, e.g. "T0:", or empty if it has no heaters |
| `heaters` | text | The current tool's heaters' current and target temperatures, e.g. " 210°C/215°C 60°C/60°C" |
| `fault` | int | 1 if any of the current tool's heaters has a fault, else 0 |

### temperature

Used by `temperature`.

| Name | Type | Description |
| --- | --- | --- |
| `tools` | list | The tools with heaters; each row also has the fields of a global `tools` row |
| `filaments` | text | The filaments the printer knows, one per line |
| `selected_tool` | text | E.g. "Selected Tool: Tool 0" |

Each row of `tools`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `number` | int | Tool number, as used by T |
| `name` | text | Display name |
| `selected` | int | 1 if this is the active tool, else 0 |
| `heater_count` | int | Number of heaters |
| `heaters` | list | One row per heater of the tool |
| `state` | text | Status key, e.g. active, standby, off |
| `status` | text | Status, translated for display |
| `extruders` | list | The tool's extruders, each with `factor` |
| `filament_shown` | int | 1 if the tool takes filament |
| `filament_text` | text | The filament picked or loaded, or "No Filament Loaded" |
| `filament_loaded` | int | 1 if filament is loaded, so it can be unloaded |
| `change_shown` | int | 1 if a filament other than the loaded one has been picked |
| `change_text` | text | "Load", or "Change" when filament is loaded |

Each row of `tools` › `heaters`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `heater` | int | Heater number |
| `name` | text | Display name, e.g. "Heater 1" or "Bed" |
| `state` | text | Status key: active, standby, off, fault, tuning, offline or unknown |
| `status` | text | Status, translated for display |
| `current` | text | Current temperature as text, one decimal place, e.g. "21.2" |
| `current_value` | int | Current temperature, whole °C, for a bar |
| `scale_min` | int | Bottom of a temperature scale, °C: the heater's minimum, or 0 if that is below 0 |
| `scale_max` | int | Top of a temperature scale, °C: the heater's maximum |
| `active` | int | Active target temperature, °C |
| `standby` | int | Standby target temperature, °C |

Each row of `tools` › `extruders`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `factor` | text | Extrusion factor, e.g. "100%" |

| Action | Description |
| --- | --- |
| `temperature.change_filament` | Load the filament picked, unloading any loaded first. From a row of `tools`. |
| `temperature.extrusion_factor` | Open the dialog to change the tool's extrusion factors. From a row of `tools`. |
| `temperature.pick_filament` | Pick the dropdown's chosen filament (`value_changed`) to load next. From a row of `tools`. |
| `temperature.unload_filament` | Unload the tool's filament. From a row of `tools`. |

### theme_preview

Used by `theme_preview`.

| Name | Type | Description |
| --- | --- | --- |
| `swatches` | list | A swatch of each colour of the theme |
| `controls_shown` | int | 1 if the palette can be changed here |
| `primary_hue` | int | Primary hue, 0-360 |
| `secondary_hue` | int | Secondary hue, 0-360 |
| `chroma` | int | Chroma, 0-0.4, in hundredths (0.02 is 2) |
| `dark` | int | 1 in dark mode |
| `primary.lightness` | int | The primary colour's OKLCH lightness, in thousandths, to show hues at (`<ds_hue_slider>`) |
| `primary.chroma` | int | The primary colour's OKLCH chroma, in thousandths, to show hues at |
| `secondary.lightness` | int | The secondary colour's OKLCH lightness, in thousandths, to show hues at |
| `secondary.chroma` | int | The secondary colour's OKLCH chroma, in thousandths, to show hues at |

Each row of `swatches`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `label` | text | The style's name, e.g. "Primary" |
| `color` | colour | Its colour, e.g. for `<bind_style_prop>` |
| `text_style` | int | For the text styles, so their names show them: 1 muted, 2 emphasis, 3 header; else 0 |

| Action | Description |
| --- | --- |
| `preview.chroma` | Set the palette's chroma to the edited value, e.g. 0.02 |
| `preview.dark` | Use the dark palette if the edited value is 1, else the light one |
| `preview.primary_hue` | Set the palette's primary hue to the edited value, 0-360 |
| `preview.secondary_hue` | Set the palette's secondary hue to the edited value, 0-360 |

### wifi

Used by `wifi_selector`.

| Name | Type | Description |
| --- | --- | --- |
| `ip` | text | E.g. "IP Address: 192.168.1.100" |
| `status` | text | How the last connection attempt went, or empty |
| `networks` | list | The networks found |
| `password_open` | int | 1 while asking for a network's password |
| `password_title` | text | The password question |
| `password` | text | The password typed |
| `keyboard` | int | 1 while the keyboard is up, so the question moves out of its way |

Each row of `networks`:

| Name | Type | Description |
| --- | --- | --- |
| `index` | int | Position in the list, from 0 |
| `ssid` | text | Network name |
| `signal` | text | Signal level, e.g. "75 dBm" |
| `known` | int | 1 if the screen has joined it before, so it can be forgotten |
| `connected` | int | 1 if the screen is on it |
| `connect_text` | text | "Disconnect" (connected), "Connect" (known) or "Join" |

| Action | Description |
| --- | --- |
| `wifi.close_password` | Stop asking for a password |
| `wifi.connect` | Disconnect from the network if on it, connect to it if known, else ask for its password to join it. From a row of `networks`. |
| `wifi.forget` | Forget the network. From a row of `networks`. |
| `wifi.join` | Join the network asked about with the password typed |
| `wifi.keyboard` | Set `keyboard` to `arg`, 1 when the keyboard comes up and 0 when it goes away |
| `wifi.password` | Store the text edited (`value_changed` on the password's text box) as the password |
| `wifi.refresh` | Scan for networks now |

## Widgets

DuetScreen's own elements, in addition to LVGL's `lv_*` widgets and the helix-xml elements (see [helix-xml's README](../libraries/helix-xml/README.md)). An element registered as `parent-child` is written inside a `parent`; `lv_obj-child` is written `<child>` inside any widget.

### ds_bar

A progress bar that draws its value as text, inside the filled part when it fits

Also takes every `lv_obj` attribute.

```xml
<ds_bar min="0" max="100" label_format="{}%" bind_value="progress"/>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_bar.png)

| Attribute | Description |
| --- | --- |
| `min` | Bottom of the range (default 0) |
| `max` | Top of the range (default 100) |
| `bind_min` | Follow an int subject for the bottom of the range, e.g. a heater's minimum |
| `bind_max` | Follow an int subject for the top of the range, e.g. a heater's maximum |
| `bind_value` | Follow an int subject for the value |
| `label_format` | fmt format for the value's text (default "{}") |

### ds_bar_marker

A value marked on a bar, such as a heater's target temperature above its current-temperature bar: a button showing the value, placed along the bar where the value falls and pointing at it with a triangle (the indicator part's background colour). Its x position is set from the value; set its y with `align` and `y`. Drag it along the bar to choose a value, raising `value_confirmed` with it when released (the marker shows the dragged value until the subject changes), or tap it to raise `clicked`; a drag doesn't raise `clicked`.

Also takes every `lv_obj` attribute.

```xml
<lv_obj width="400" height="100" scrollable="false">
	<ds_bar name="current" bind_value="current_value" bind_min="scale_min" bind_max="scale_max"
			width="100%" height="15" align="center"/>
	<ds_bar_marker name="target" bar="current" bind_value="active" bind_min="scale_min" bind_max="scale_max"
				   label_format="{} °C" align="left_mid" y="-30">
		<style name="input"/>
		<action trigger="clicked" name="heaters.set_active"/>
		<action trigger="value_confirmed" name="heaters.set_active"/>
	</ds_bar_marker>
</lv_obj>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_bar_marker.png)

| Attribute | Description |
| --- | --- |
| `bar` | Name of the sibling bar it marks, written before the marker. The bar spans its parent's width (e.g. width="100%"), as the marker is placed by percentages of that width |
| `min` | Bottom of the scale the value is on (default 0) |
| `max` | Top of the scale the value is on (default 100) |
| `bind_min` | Follow an int subject for the bottom of the scale |
| `bind_max` | Follow an int subject for the top of the scale |
| `bind_value` | Follow an int subject for the value marked |
| `label_format` | fmt format for the value's text (default "{}") |

### ds_button

A button showing text, an icon or both

Also takes every `lv_obj` attribute.

```xml
<ds_button translation_tag="fan.max" icon="fan.png">
	<action trigger="clicked" name="fans.set_speed" arg="100"/>
</ds_button>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_button.png)

| Attribute | Description |
| --- | --- |
| `text` | The text shown, if there is no `translation_tag` |
| `translation_tag` | Translation key for the text; follows language changes |
| `bind_text` | Follow a text or int subject for the text |
| `bind_text-fmt` | printf format for `bind_text`, e.g. "%d°C" (default "%s" for text, "%d" for an int) |
| `icon` | Icon file from the icon set, e.g. "fan.png" |
| `checkable` | "true" to make the button toggle its `checked` state when clicked |
| `long_mode` | What the label does with text too long for it: wrap, dots, scroll, scroll_circular or clip |
| `flash_on` | Flash the button briefly, in its `user_1` state, whenever this subject changes, e.g. `responses.count`; style the flash with `<style … selector="user_1"/>` |

### ds_dropdown

A drop-down list whose options and text follow subjects. The option chosen reaches a `value_changed` action as its text (ActionArgs::text()) and its index (ActionArgs::value()).

Also takes every `lv_dropdown` attribute.

```xml
<ds_dropdown bind_options="filaments" bind_text="filament_text">
	<action trigger="value_changed" name="temperature.pick_filament"/>
</ds_dropdown>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_dropdown.png)

| Attribute | Description |
| --- | --- |
| `bind_options` | Follow a text subject holding the options, one per line |
| `bind_text` | Follow a text subject shown on the button instead of the chosen option, e.g. what is loaded |
| `bind_selected` | Follow an int subject for the index of the option shown chosen, kept when the options change |

### ds_graph

A line graph of a history, with a legend whose buttons show and hide series. Series are coloured by position from LVGL's palette (red, pink, purple, …). The graph starts with what was recorded before it was built, and follows series being added, removed and renamed.

Also takes every `lv_obj` attribute.

```xml
<ds_graph history="temperatures" seconds="60" y_min="0" y_max="300" bind_y_max="temperatures.limit"
		  width="400" height="250"/>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_graph.png)

| Attribute | Description |
| --- | --- |
| `history` | The history shown (required), looked up like a list: the enclosing rows, the view's presenter, then the globals |
| `seconds` | The X axis shows the last this-many seconds, labelled -seconds to 0 (default 60) |
| `y_min` | Bottom of the Y axis (default 0) |
| `y_max` | Top of the Y axis (default 100) |
| `bind_y_max` | Follow an int subject that raises the top of the Y axis above `y_max`, e.g. to fit a heater's limit |

### ds_heatmap

A colour map of a surface, such as a bed's height map, with X and Y scales, a grid, a title, and the colour scale beside it

Also takes every `lv_obj` attribute.

```xml
<ds_heatmap bind_surface="surface" bind_title="title" resolution="200" width="400" height="300"/>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_heatmap.png)

| Attribute | Description |
| --- | --- |
| `bind_surface` | The surface subject to show; redrawn whenever it changes |
| `bind_title` | Follow a text subject for the title shown above the map |
| `resolution` | Pixels the map is drawn with along each side, scaled to fit (default 200) |

### ds_hue_slider

A `<ds_slider>` for a hue, 0 to 360, whose track shows every hue at a lightness and chroma, so the slider is a colour picker, e.g. for a theme's primary colour. `min` and `max` default to 0 and 360.

Also takes every `ds_slider` attribute.

```xml
<ds_hue_slider bind_value="primary_hue" bind_lightness="primary.lightness" bind_chroma="primary.chroma"
			   width="100%"/>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_hue_slider.png)

| Attribute | Description |
| --- | --- |
| `bind_lightness` | Follow an int subject for the OKLCH lightness the hues are shown at, in thousandths (0.65 is 650) |
| `bind_chroma` | Follow an int subject for the OKLCH chroma the hues are shown at, in thousandths |

### ds_icon

An icon from the icon set chosen in the settings, coloured by the theme. It follows icon set changes.

Also takes every `lv_image` attribute.

```xml
<ds_icon icon="folder.png" width="72" height="72"/>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_icon.png)

| Attribute | Description |
| --- | --- |
| `icon` | Icon file from the icon set, e.g. "folder.png" |

### ds_keyboard

An on-screen keyboard with DuetScreen's key layouts. A `<ds_textarea>` brings it up and types into it, so it is usually written `hidden="true"`.

Also takes every `lv_obj` attribute.

```xml
<ds_keyboard name="console_keyboard" mode="text_upper" width="100%" height="200"/>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_keyboard.png)

| Attribute | Description |
| --- | --- |
| `mode` | Key layout: `text` (default), `text_upper` or `number` |

### ds_list

A container with one component per row of a list. Each item is built in a scope of its own holding its row's fields, so names are found in the row first, then where the list is written. With `windowed`, only the rows on screen are built, so a folder of a thousand files costs a screenful of widgets; every item then has the same size, and the list places them itself (its own layout is ignored), with its style_pad_row and style_pad_column as the gaps.

Also takes every `lv_obj` attribute.

```xml
<ds_list list="files" item="file_row" windowed="true" item_height="90" flow="column"/>
```

| Attribute | Description |
| --- | --- |
| `list` | The list shown (required), looked up like a subject: the enclosing rows first (so a tool card can show `list="heaters"`), then the view's presenter, then the globals |
| `item` | The component built for each row (required) |
| `windowed` | "true" to build only the rows on screen, building and deleting rows as the list scrolls (default false) |
| `item_height` | Windowed: each item's height in pixels (required when windowed) |
| `item_width` | Windowed: each item's width in pixels, or `content` for each item's own width (column flow only; the list scrolls sideways); the list's full width if not given |
| `flow` | Windowed: `column` for one item per line (default), `row_wrap` for as many per line as fit |
| `follow_end` | "true" to keep a list that isn't windowed showing its last rows as rows are added, like a log, unless the user has scrolled back (default false) |

### ds_modal

A dialog over the main window with a dimmed backdrop. Its subject alone decides whether it is open, so a presenter opens and closes it by setting a subject. Closed any other way (tapped outside, the back button, navigation closing every modal), it raises `dismissed` so the presenter can update the subject; closing it by its subject raises nothing. The content is laid out in the dialog's card, which the lv_obj attributes apply to. Unless `in_place`, it is deleted with the widget it was written in.

Also takes every `lv_obj` attribute.

```xml
<ds_modal name="file_dialog" bind_open="dialog" width="70%" height="content">
	<action trigger="dismissed" name="files.close_dialog"/>
	…the dialog's content…
</ds_modal>
```

| Attribute | Description |
| --- | --- |
| `bind_open` | Open while this subject is non-zero (int) or non-empty (text), or, with `open_value`, equal to it |
| `open_value` | The value `bind_open` must have for the dialog to be open |
| `blocking` | "true" so that tapping outside doesn't close it (default false) |
| `in_place` | "true" to stay in the widget it is written in rather than move to the main window, so the backdrop covers only that widget, e.g. alerts covering the whole screen (default false) |

### ds_plot

An X/Y area with scales and a grid, such as the bed with its objects, whose content is widgets placed by X/Y position. `<ds_plot_layer>`s written inside it are kept over its drawing area, and `<ds_plot_rect>`s inside a layer are kept at their X/Y bounds.

Also takes every `lv_obj` attribute.

```xml
<ds_plot title_tag="object_cancel.current_object_prefix" title_style="text_emphasis" bind_detail="current"
		 bind_x_min="x_min" bind_x_max="x_max" bind_y_min="y_min" bind_y_max="y_max" width="400" height="300">
	<ds_plot_layer name="layer">
		<ds_list list="objects" item="job_object_rect" width="100%" height="100%"/>
	</ds_plot_layer>
</ds_plot>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_plot.png)

| Attribute | Description |
| --- | --- |
| `title` | The title above the area, if there is no `title_tag` |
| `title_tag` | Translation key for the title |
| `bind_title` | Follow a text subject for the title |
| `title_style` | A theme style for the title, e.g. `text_emphasis` |
| `bind_detail` | Follow a text subject shown after the title |
| `x_min` | Left of the span (default 0) |
| `x_max` | Right of the span (default 100) |
| `y_min` | Bottom of the span (default 0) |
| `y_max` | Top of the span (default 100) |
| `bind_x_min` | Follow an int subject for `x_min` |
| `bind_x_max` | Follow an int subject for `x_max` |
| `bind_y_min` | Follow an int subject for `y_min` |
| `bind_y_max` | Follow an int subject for `y_max` |
| `resolution` | Pixels the grid is drawn with along each side, scaled to fit (default 200) |

### ds_plot_layer

A layer written inside a `<ds_plot>`, kept exactly over its drawing area, for widgets placed by X/Y position such as `<ds_plot_rect>`s. It doesn't scroll and has no padding; the plot sets its position and size.

Also takes every `lv_obj` attribute.

### ds_plot_rect

A rectangle kept at its X/Y bounds over a `<ds_plot>`, e.g. an object on the bed. It is written inside a `<ds_plot_layer>` of the plot, directly or deeper (in a list row, say), and hidden while its bounds are outside the plot's span. It sets its own position and size.

Also takes every `lv_obj` attribute.

```xml
<ds_plot_rect bind_x1="x1" bind_y1="y1" bind_x2="x2" bind_y2="y2"/>
```

| Attribute | Description |
| --- | --- |
| `x1` | X of one corner, in the plot's units |
| `y1` | Y of that corner |
| `x2` | X of the opposite corner |
| `y2` | Y of the opposite corner |
| `bind_x1` | Follow an int subject for `x1` |
| `bind_y1` | Follow an int subject for `y1` |
| `bind_x2` | Follow an int subject for `x2` |
| `bind_y2` | Follow an int subject for `y2` |

### ds_slider

A slider with a label, its value and -/+ buttons, and optionally a number pad. Raises `value_confirmed` with the value when the user finishes an edit (releases it, presses -/+ or types a value), or with `send="change"` on every movement. Values written by `bind_value` are not edits and raise nothing, so a view can show live data and send changes without echoing the data back to the printer.

Also takes every `lv_obj` attribute.

```xml
<ds_slider label_tag="settings.brightness" min="0" max="100" increment="1" decimals="0" bind_value="brightness"
		   numberpad="true" send="change" width="100%">
	<action trigger="value_confirmed" name="settings.brightness"/>
</ds_slider>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_slider.png)

| Attribute | Description |
| --- | --- |
| `min` | Bottom of the range, may have a fraction (default 0) |
| `max` | Top of the range, may have a fraction (default 100) |
| `increment` | Step of the -/+ buttons, may have a fraction, e.g. "0.01" (default 1) |
| `decimals` | Decimal places the value is shown with (default 2) |
| `label` | The label, if there is no `label_tag` |
| `label_tag` | Translation key for the label |
| `bind_label` | Follow a text subject for the label |
| `label_shown` | "false" to hide the label, which still heads the number pad (default true) |
| `bind_value` | Follow an int subject for the value |
| `value_scale` | How much `bind_value` is scaled by, for a value with a fraction, e.g. a chroma of 0.02 held as 2 with value_scale="100" (default 1) |
| `send` | `confirmed` to raise `value_confirmed` when an edit is finished (default), or `change` on every movement, for a setting seen as it changes such as brightness |
| `out_of_range` | Let a typed value go past the slider's `upper` end, `lower` end or `both`, e.g. a timeout longer than the slider reaches |
| `numberpad` | "true" to type the value on the number pad (the view registered as `numberpad`) when the value is tapped (default false) |

### ds_tabs

A row of tab buttons over the selected tab's content. Each tab is a `<ds_tabs-tab>` inside it; the first starts selected.

Also takes every `lv_obj` attribute.

```xml
<ds_tabs width="100%" height="200">
	<ds_tabs-tab name="macros" title_tag="file.macros">
		<lv_label text="The first tab's content"/>
	</ds_tabs-tab>
	<ds_tabs-tab name="jobs" title_tag="file.jobs" bind_disabled="machine.job_active">
		<lv_label text="The second tab's content"/>
	</ds_tabs-tab>
</ds_tabs>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_tabs.png)

### ds_tabs-tab

One tab of a `<ds_tabs>`, written inside it. Its content is the tab's page; the lv_obj attributes apply to that page.

Also takes every `lv_obj` attribute.

| Attribute | Description |
| --- | --- |
| `title` | The tab button's text, if there is no `title_tag` |
| `title_tag` | Translation key for the tab button's text |
| `bind_disabled` | Disable the tab's button while this int subject is non-zero |
| `select_on` | Select the tab whenever this subject changes to a non-empty text or non-zero int, e.g. the status tab when a job starts: `select_on="job.file"` |

### ds_textarea

A text box that brings up an on-screen keyboard while it is being edited. The keyboard's own close key always puts it away. Raises `focused`, `defocused`, `ready` and `cancel` as the edit starts and ends, and `value_changed` on every edit; handlers read the text with ActionArgs::text().

Also takes every `lv_obj` attribute.

```xml
<ds_textarea name="input" one_line="true" bind_text="alert.input" bind_keyboard_mode="alert.keyboard_mode"
			 width="300">
	<action trigger="value_changed" name="alert.input"/>
</ds_textarea>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_textarea.png)

| Attribute | Description |
| --- | --- |
| `bind_text` | Follow a text subject: the box shows its value whenever it changes to something other than what is in the box, so a presenter that keeps the subject equal to the edits can replace the text at any time |
| `bind_accepted_chars` | Follow a text subject for the characters that may be typed; empty for any |
| `keyboard_mode` | Keyboard layout to show: `text` (default), `text_upper` or `number` |
| `bind_keyboard_mode` | Follow a text subject for the keyboard layout, as `keyboard_mode` |
| `keyboard` | The keyboard to use: the nearest lv_keyboard of that name around the box (e.g. one in the same view), else the view of that name (default `keyboard`, the shared one); `none` for a box edited another way, such as a number pad |
| `hide_keyboard_on_ready` | Put the keyboard away when its OK key is pressed (default true) |
| `hide_keyboard_on_defocus` | Put the keyboard away when the box loses focus (default true) |
| `one_line` | "true" for a single line of text |
| `password_mode` | "true" to show dots for the characters, e.g. for a password |
| `placeholder` | Text shown while the box is empty |
| `placeholder_tag` | Translation key for the placeholder, instead of `placeholder` |

### ds_thumbnail

An image as tall as its parent's content and as wide as its aspect ratio needs, so a thumbnail beside some text is as large as it can be without making the row taller. With no image it takes no width.

Also takes every `lv_image` attribute.

```xml
<ds_thumbnail bind_src="thumbnail" height="100%"/>
```

| Attribute | Description |
| --- | --- |
| `bind_src` | Follow a text subject holding an image path; empty shows no image |

### ds_toggle

An on/off setting, drawn as the build's toggle (a check box or a switch), so every setting looks the same. Raises `value_changed` when it is changed; the action's value is 1 when turned on and 0 when off.

Also takes every `lv_obj` attribute.

```xml
<ds_toggle>
	<bind_state_if_eq subject="confirmations" state="checked" ref_value="1"/>
	<action trigger="value_changed" name="settings.confirmations"/>
</ds_toggle>
```

![The example](../tests/ref_imgs/xml_ui_api/themes/duetscreen/widgets/ds_toggle.png)

### ds_view

A view inside a screen, e.g. a page of the home screen, registered with navigation as its `name` (default: its `view`) so it can be opened with `nav.open`. A view starts hidden, like any screen that isn't open. Every lv_obj attribute works except `hidden`, which navigation owns.

Also takes every `lv_obj` attribute.

```xml
<ds_view name="dashboard_jobs" view="file_browser" presenter="dashboard_jobs"/>
```

| Attribute | Description |
| --- | --- |
| `view` | The XML component shown (with its presenter), or if there is no such component, a C++ view |
| `presenter` | Show the component with this registered presenter instead of the one it names, so one component can show different data in different places |
| `home` | "true" to make it a home screen: shown when nothing else is open (default false) |
| `shown` | "true" to show it without taking part in navigation, for always-present parts such as a status bar or the page in a tab: shown, and its presenter active, whenever what it is in is shown (default false) |

### lv_obj-action

Written `<action>` inside any widget: run the named action when the widget raises the trigger event. The action is looked for in the presenters of the views the widget is in, innermost first, then in the global actions.

```xml
<action trigger="value_confirmed" name="gcode" arg="M106 P{number} S{value}"/>
```

| Attribute | Description |
| --- | --- |
| `name` | The action to run (required) |
| `trigger` | The event that runs it (default `clicked`): an LVGL event name such as `value_changed` or `long_pressed`, `value_confirmed` (a DuetScreen widget's edit is finished) or `dismissed` (a `<ds_modal>` was closed by the user) |
| `arg` | Passed to the handler. `{value}` is replaced by the edited value, and `{name}` by the current value of the subject `name`, looked up from where the `<action>` is written |

## Components

The components of the built-in bundle. A user bundle replaces one by giving its own the same name; a component is used as an element (`<fan_card/>`) or as a list's `item`.

| Component | Presenter | Props | Used by | File |
| --- | --- | --- | --- | --- |
| `alert_axis` |  |  | `notifications` | [alert_axis.xml](../assets/ui/components/alert_axis.xml) |
| `alert_choice` |  |  | `notifications` | [alert_choice.xml](../assets/ui/components/alert_choice.xml) |
| `axis_position` |  |  | `job_status` | [axis_position.xml](../assets/ui/components/axis_position.xml) |
| [`babystep`](#control_screen) | `babystep` |  | `job_status`, `move` | [babystep.xml](../assets/ui/components/babystep.xml) |
| `babystep_step` |  |  | `babystep` | [babystep_step.xml](../assets/ui/components/babystep_step.xml) |
| `confirm_dialog` |  | `open`, `title_tag`, `text_tag`, `confirm`, `close` | `job_status` | [confirm_dialog.xml](../assets/ui/components/confirm_dialog.xml) |
| [`console`](#console) | `console` |  | `home_screen` | [console.xml](../assets/ui/screens/console.xml) |
| `console_gcode` |  |  | `console` | [console_gcode.xml](../assets/ui/components/console_gcode.xml) |
| `console_line` |  |  | `console` | [console_line.xml](../assets/ui/components/console_line.xml) |
| [`control_screen`](#control_screen) |  |  | `home_screen` | [control_screen.xml](../assets/ui/screens/control_screen.xml) |
| [`dashboard`](#home_screen) |  |  | `home_screen` | [dashboard.xml](../assets/ui/screens/dashboard.xml) |
| [`extruder`](#temperature) | `extruder` |  | `temperature` | [extruder.xml](../assets/ui/components/extruder.xml) |
| `fan_card` |  |  | `fan_screen` | [fan_card.xml](../assets/ui/components/fan_card.xml) |
| [`fan_screen`](#fan_screen) | `fans` |  | `control_screen` | [fan_screen.xml](../assets/ui/screens/fan_screen.xml) |
| [`file_browser`](#files_screen) |  |  | `dashboard`, `files_screen` | [file_browser.xml](../assets/ui/components/file_browser.xml) |
| `file_crumb` |  |  | `file_browser` | [file_crumb.xml](../assets/ui/components/file_crumb.xml) |
| `file_dialog` |  |  | `file_browser` | [file_dialog.xml](../assets/ui/components/file_dialog.xml) |
| `file_row` |  |  | `file_browser` | [file_row.xml](../assets/ui/components/file_row.xml) |
| `file_tile` |  |  | `file_browser` | [file_tile.xml](../assets/ui/components/file_tile.xml) |
| [`files_screen`](#files_screen) |  |  | `home_screen` | [files_screen.xml](../assets/ui/screens/files_screen.xml) |
| `heater_card` |  |  | `tool_list` | [heater_card.xml](../assets/ui/components/heater_card.xml) |
| `heater_row` |  |  | `heater_card`, `tool_card` | [heater_row.xml](../assets/ui/components/heater_row.xml) |
| [`heightmap`](#heightmap) | `heightmap` |  | `control_screen` | [heightmap.xml](../assets/ui/screens/heightmap.xml) |
| `heightmap_file` |  |  | `heightmap` | [heightmap_file.xml](../assets/ui/components/heightmap_file.xml) |
| [`home_screen`](#home_screen) |  |  |  | [home_screen.xml](../assets/ui/screens/home_screen.xml) |
| `job_object_item` |  |  | `object_cancel` | [job_object_item.xml](../assets/ui/components/job_object_item.xml) |
| `job_object_rect` |  |  | `object_cancel` | [job_object_rect.xml](../assets/ui/components/job_object_rect.xml) |
| [`job_status`](#job_status) | `job_status` |  | `dashboard` | [job_status.xml](../assets/ui/components/job_status.xml) |
| [`move`](#control_screen) | `move` |  | `control_screen` | [move.xml](../assets/ui/screens/move.xml) |
| `move_axis` |  |  | `move` | [move_axis.xml](../assets/ui/components/move_axis.xml) |
| `move_distance` |  |  | `move` | [move_distance.xml](../assets/ui/components/move_distance.xml) |
| `move_feedrate` |  |  | `move` | [move_feedrate.xml](../assets/ui/components/move_feedrate.xml) |
| `move_xy` |  |  | `move` | [move_xy.xml](../assets/ui/components/move_xy.xml) |
| [`notifications`](#home_screen) | `notifications` |  | `home_screen` | [notifications.xml](../assets/ui/components/notifications.xml) |
| [`object_cancel`](#object_cancel) | `object_cancel` |  | `control_screen` | [object_cancel.xml](../assets/ui/screens/object_cancel.xml) |
| `question_dialog` |  | `open`, `title`, `text`, `confirm`, `close` | `move`, `object_cancel`, `settings_general` | [question_dialog.xml](../assets/ui/components/question_dialog.xml) |
| `setting_dropdown` |  | `options`, `selected`, `action` | `settings_connection`, `settings_developer`, `settings_display`, `settings_general` | [setting_dropdown.xml](../assets/ui/components/setting_dropdown.xml) |
| [`settings_connection`](#settings_connection) | `settings_connection` |  | `settings_screen` | [settings_connection.xml](../assets/ui/screens/settings_connection.xml) |
| [`settings_developer`](#settings_developer) | `settings_developer` |  | `settings_screen` | [settings_developer.xml](../assets/ui/screens/settings_developer.xml) |
| [`settings_display`](#settings_display) | `settings_display` |  | `settings_screen` | [settings_display.xml](../assets/ui/screens/settings_display.xml) |
| [`settings_general`](#settings_screen) | `settings_general` |  | `settings_screen` | [settings_general.xml](../assets/ui/screens/settings_general.xml) |
| [`settings_screen`](#settings_screen) |  |  | `home_screen` | [settings_screen.xml](../assets/ui/screens/settings_screen.xml) |
| [`side_bar`](#home_screen) |  |  | `home_screen` | [side_bar.xml](../assets/ui/components/side_bar.xml) |
| [`status_bar`](#home_screen) | `status_bar` |  | `home_screen` | [status_bar.xml](../assets/ui/components/status_bar.xml) |
| [`temperature`](#temperature) | `temperature` |  | `control_screen` | [temperature.xml](../assets/ui/screens/temperature.xml) |
| `temperature_extruder` |  |  | `temperature_tool` | [temperature_extruder.xml](../assets/ui/components/temperature_extruder.xml) |
| `temperature_heater` |  |  | `temperature_heater_card`, `temperature_tool` | [temperature_heater.xml](../assets/ui/components/temperature_heater.xml) |
| `temperature_heater_card` |  |  | `temperature` | [temperature_heater_card.xml](../assets/ui/components/temperature_heater_card.xml) |
| `temperature_tool` |  |  | `temperature` | [temperature_tool.xml](../assets/ui/components/temperature_tool.xml) |
| [`theme_preview`](#settings_display) | `theme_preview` |  | `settings_display` | [theme_preview.xml](../assets/ui/components/theme_preview.xml) |
| `theme_swatch` |  |  | `theme_preview` | [theme_swatch.xml](../assets/ui/components/theme_swatch.xml) |
| `tool_card` |  |  | `tool_list` | [tool_card.xml](../assets/ui/components/tool_card.xml) |
| [`tool_list`](#home_screen) |  |  | `dashboard` | [tool_list.xml](../assets/ui/components/tool_list.xml) |
| `value_choice` |  |  | `value_selector` | [value_choice.xml](../assets/ui/components/value_choice.xml) |
| `value_selector` |  | `selector`, `title`, `value`, `choices` | `extruder` | [value_selector.xml](../assets/ui/components/value_selector.xml) |
| `wifi_network` |  |  | `wifi_selector` | [wifi_network.xml](../assets/ui/components/wifi_network.xml) |
| [`wifi_selector`](#settings_connection) | `wifi` |  | `settings_connection` | [wifi_selector.xml](../assets/ui/components/wifi_selector.xml) |

C++ views that `<ds_view view="…">` can show where there is no component of the name: `container`, `estop`, `extrusion_factor`, `job_extrusion_factor`, `keyboard`, `numberpad`, `speed_factor`

### Views

The components shown as views of their own, drawn with sample printer data in the `duetscreen` theme. A view that shows in another's picture (the status bar in the home screen's) is pictured only there.

#### console

![console](../tests/ref_imgs/xml_ui_api/views/console.png)

#### control_screen

Also shows `babystep`, `move`.

![control_screen](../tests/ref_imgs/xml_ui_api/views/control_screen.png)

#### fan_screen

![fan_screen](../tests/ref_imgs/xml_ui_api/views/fan_screen.png)

#### files_screen

Also shows `file_browser`.

![files_screen](../tests/ref_imgs/xml_ui_api/views/files_screen.png)

#### heightmap

![heightmap](../tests/ref_imgs/xml_ui_api/views/heightmap.png)

#### home_screen

Also shows `dashboard`, `notifications`, `side_bar`, `status_bar`, `tool_list`.

![home_screen](../tests/ref_imgs/xml_ui_api/views/home_screen.png)

#### job_status

![job_status](../tests/ref_imgs/xml_ui_api/views/job_status.png)

#### object_cancel

![object_cancel](../tests/ref_imgs/xml_ui_api/views/object_cancel.png)

#### settings_connection

Also shows `wifi_selector`.

![settings_connection](../tests/ref_imgs/xml_ui_api/views/settings_connection.png)

#### settings_developer

![settings_developer](../tests/ref_imgs/xml_ui_api/views/settings_developer.png)

#### settings_display

Also shows `theme_preview`.

![settings_display](../tests/ref_imgs/xml_ui_api/views/settings_display.png)

#### settings_screen

Also shows `settings_general`.

![settings_screen](../tests/ref_imgs/xml_ui_api/views/settings_screen.png)

#### temperature

Also shows `extruder`.

![temperature](../tests/ref_imgs/xml_ui_api/views/temperature.png)

## Theme

Themes are described in [XML_UI.md](XML_UI.md#themes), and every built-in theme, with what it changes and how it looks, in [XML_UI_THEMES.md](XML_UI_THEMES.md). Everything here follows theme changes.

### Styles

Used with `<style name="…"/>`. The definition is the one in the `base` theme, which every theme extends; other themes change some of them.

The swatch is a box with some text, with the style added, in the `duetscreen` theme; a style that changes nothing on its own (a state's, or a part's) has none.

| Style | Swatch | Definition in `base` |
| --- | --- | --- |
| `base` |  | bg_opa=0 border_post=true |
| `bg_dark` |  | bg_color=#bg_dark bg_opa=100% bg_grad_dir=none |
| `bg` | ![bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg.png) | bg_color=#bg bg_opa=100% bg_grad_dir=none |
| `bg_light` | ![bg_light](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_light.png) | bg_color=#bg_light bg_opa=100% bg_grad_dir=none |
| `bg_color_primary` | ![bg_color_primary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_primary.png) | bg_color=#primary bg_opa=100% bg_grad_dir=none |
| `bg_color_primary_muted` | ![bg_color_primary_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_primary_muted.png) | bg_color=#primary_muted bg_opa=100% bg_grad_dir=none |
| `bg_color_secondary` | ![bg_color_secondary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_secondary.png) | bg_color=#secondary bg_opa=100% bg_grad_dir=none |
| `bg_color_secondary_muted` | ![bg_color_secondary_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_secondary_muted.png) | bg_color=#secondary_muted bg_opa=100% bg_grad_dir=none |
| `bg_color_success` | ![bg_color_success](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_success.png) | bg_color=#success bg_opa=100% bg_grad_dir=none |
| `bg_color_warning` | ![bg_color_warning](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_warning.png) | bg_color=#warning bg_opa=100% bg_grad_dir=none |
| `bg_color_error` | ![bg_color_error](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_error.png) | bg_color=#error bg_opa=100% bg_grad_dir=none |
| `bg_color_header` | ![bg_color_header](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_header.png) | bg_color=0xffffff bg_opa=10% |
| `bg_color_list_item` | ![bg_color_list_item](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_color_list_item.png) | bg_color=#bg bg_opa=100% bg_grad_color=#bg_light |
| `bg_modal` | ![bg_modal](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_modal.png) | bg_color=#bg bg_opa=70% |
| `text` | ![text](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text.png) | text_color=#text text_font=font_normal |
| `text_muted` | ![text_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_muted.png) | text_color=#text_muted text_font=font_subdued |
| `text_emphasis` | ![text_emphasis](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_emphasis.png) | text_color=#text text_font=font_emphasis |
| `text_header` | ![text_header](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_header.png) | text_color=#text_header text_font=font_header |
| `text_warning` | ![text_warning](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/text_warning.png) | text_color=#error text_font=font_emphasis |
| `border` | ![border](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border.png) | border_color=#border border_width=#border_width border_opa=100% border_side=full |
| `border_muted` | ![border_muted](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_muted.png) | border_color=#border_muted border_width=#border_width border_opa=100% border_side=full |
| `border_highlight` | ![border_highlight](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_highlight.png) | border_color=#highlight border_width=#border_width border_opa=100% border_side=full |
| `border_color_primary` | ![border_color_primary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_color_primary.png) | border_color=#primary border_width=#border_width border_opa=100% border_side=full |
| `border_color_secondary` | ![border_color_secondary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/border_color_secondary.png) | border_color=#secondary border_width=#border_width border_opa=100% border_side=full |
| `border_color_card` |  | *(not set)* |
| `screen` |  | bg_color=#bg_dark bg_opa=100% |
| `scrollbar` | ![scrollbar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/scrollbar.png) | bg_color=#bg_light border_color=#border border_width=1 radius=circle pad_all=7dpx width=5dpx bg_opa=40% |
| `scrollbar_scrolled` | ![scrollbar_scrolled](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/scrollbar_scrolled.png) | bg_opa=100% |
| `card` | ![card](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/card.png) | radius=#radius pad_all=#pad pad_row=#pad_small pad_column=#pad_small bg_opa=100% bg_color=#bg border_color=#border border_width=#border_width border_opa=100% border_side=full |
| `tab_card` |  | *(not set)* |
| `modal` |  | *(not set)* |
| `btn` | ![btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btn.png) | radius=#radius bg_opa=100% bg_color=#primary |
| `btn_checked` | ![btn_checked](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btn_checked.png) | merge=border_highlight bg_grad_dir=none |
| `icon` |  | *(not set)* |
| `icon_recolor` | ![icon_recolor](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/icon_recolor.png) | recolor=#text recolor_opa=100% |
| `pressed` | ![pressed](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/pressed.png) | recolor=0x000000 recolor_opa=35 |
| `disabled` | ![disabled](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/disabled.png) | recolor=#bg recolor_opa=50% |
| `pad_base` |  | pad_all=#pad pad_row=#pad pad_column=#pad |
| `pad_zero` |  | pad_all=0 pad_row=0 pad_column=0 |
| `pad_tiny` |  | pad_all=#pad_tiny pad_row=#pad_tiny pad_column=#pad_tiny |
| `pad_small` |  | pad_all=#pad_small pad_gap=#pad_small |
| `pad_normal` |  | pad_all=#pad pad_row=#pad pad_column=#pad |
| `pad_large` |  | pad_all=#pad_large pad_row=#pad_large pad_column=#pad_large |
| `pad_gap` |  | pad_row=10dpx pad_column=10dpx |
| `line_space_large` |  | text_line_space=20dpx |
| `text_align_center` |  | text_align=center |
| `outline_primary` | ![outline_primary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/outline_primary.png) | outline_color=#primary outline_width=#outline_width outline_pad=#outline_width outline_opa=50% |
| `outline_secondary` | ![outline_secondary](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/outline_secondary.png) | outline_color=#secondary outline_width=#outline_width outline_opa=50% |
| `circle` |  | radius=circle |
| `no_radius` |  | radius=0 |
| `no_border` |  | border_width=0 |
| `clip_corner` |  | clip_corner=true border_post=true |
| `rotary_scroll` |  | rotary_sensitivity=8192 |
| `grow` |  | transform_width=3dpx transform_height=3dpx |
| `transition_delayed` |  | *(not set)* |
| `transition_normal` |  | *(not set)* |
| `anim` |  | anim_duration=200 |
| `anim_fast` |  | anim_duration=120 |
| `actionBtn` | ![actionBtn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/actionBtn.png) | bg_opa=100% bg_color=#secondary |
| `input` | ![input](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/input.png) | border_color=#border border_width=1 pad_hor=20 pad_ver=0 text_align=center |
| `long_press` |  | *(not set)* |
| `draggable` |  | transition_props=transform_width transform_height transition_duration=200ms transition_easing=ease_in_out transition_delay=100ms |
| `dragging` | ![dragging](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/dragging.png) | transform_width=20 transform_height=20 border_color=0xffffff border_opa=30% border_width=20 transition_props=outline_width transition_duration=#transition_time transition_easing=ease_in_out |
| `drag_complete` | ![drag_complete](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/drag_complete.png) | outline_color=0xffffff outline_width=3 |
| `knob` | ![knob](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/knob.png) | bg_color=#primary bg_opa=100% pad_all=6dpx radius=circle |
| `arc_indic` |  | arc_color=#highlight arc_width=15dpx arc_rounded=true |
| `arc_indic_primary` |  | arc_color=#primary |
| `bar` | ![bar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bar.png) | bg_color=#highlight bg_opa=100% radius=circle |
| `bar_indic` |  | radius=circle |
| `btnm_bg` |  | *(not set)* |
| `btnm_btn` | ![btnm_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btnm_btn.png) | merge=border_muted bg_color=#bg_light bg_grad_dir=none shadow_width=0 radius=#radius |
| `btnm_btn_checked` | ![btnm_btn_checked](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/btnm_btn_checked.png) | bg_color=#primary bg_opa=100% text_color=#text |
| `chart_series` |  | line_width=3dpx radius=3dpx width=8dpx height=8dpx pad_column=2dpx |
| `chart_indic` | ![chart_indic](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/chart_indic.png) | radius=circle width=8dpx height=8dpx bg_color=#primary bg_opa=100% |
| `chart_bg` | ![chart_bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/chart_bg.png) | border_width=#border_width border_color=#highlight border_opa=100% border_post=false pad_column=10dpx line_color=#highlight |
| `dropdown` |  | pad_ver=#pad |
| `dropdown_list` |  | max_height=260 |
| `cb_marker` | ![cb_marker](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/cb_marker.png) | pad_all=3dpx border_width=#border_width border_color=#primary bg_color=#bg bg_opa=100% radius=2 |
| `cb_marker_checked` | ![cb_marker_checked](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/cb_marker_checked.png) | bg_image_src=symbol_ok |
| `bg_switch` | ![bg_switch](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bg_switch.png) | bg_color=#bg_light bg_opa=100% radius=circle |
| `switch_knob` |  | pad_all=-4dpx bg_color=#text radius=circle |
| `line` |  | line_width=1 line_color=#highlight |
| `table` |  | radius=0 |
| `table_cell` | ![table_cell](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/table_cell.png) | bg_color=#bg bg_opa=100% text_color=#text border_width=1dpx border_color=#highlight border_side=top\|bottom |
| `text_area` |  | pad_all=#pad |
| `ta_cursor` | ![ta_cursor](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/ta_cursor.png) | border_color=#text border_width=2dpx pad_left=-1dpx border_side=left anim_duration=400 |
| `ta_placeholder` | ![ta_placeholder](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/ta_placeholder.png) | text_color=#text_muted |
| `calendar_btnm_bg` |  | pad_all=#pad_small pad_gap=1 |
| `calendar_btnm_day` | ![calendar_btnm_day](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/calendar_btnm_day.png) | border_width=1dpx border_color=#highlight bg_color=#bg bg_opa=20% |
| `calendar_header` |  | pad_hor=#pad_small pad_top=#pad_small pad_bottom=#pad_tiny pad_gap=#pad_small |
| `canvas` | ![canvas](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/canvas.png) | merge=bg merge=border |
| `menu_bg` |  | pad_all=0 pad_gap=0 radius=0 clip_corner=true border_side=none |
| `menu_cont` |  | pad_hor=#pad_small pad_ver=#pad_small pad_gap=#pad_small border_width=1dpx border_opa=10% border_color=#text border_side=none |
| `menu_sidebar_cont` | ![menu_sidebar_cont](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_sidebar_cont.png) | pad_all=0 pad_gap=0 border_width=1dpx border_opa=10% border_color=#text border_side=right |
| `menu_main_cont` |  | pad_all=0 pad_gap=0 |
| `menu_page` |  | pad_hor=0 pad_gap=0 |
| `menu_header_cont` |  | pad_hor=#pad_small pad_ver=#pad_tiny pad_gap=#pad_small |
| `menu_header_btn` | ![menu_header_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_header_btn.png) | pad_hor=#pad_tiny pad_ver=#pad_tiny shadow_opa=0 bg_opa=0 text_color=#text |
| `menu_section` | ![menu_section](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_section.png) | radius=#radius clip_corner=true bg_opa=100% bg_color=#bg text_color=#text |
| `menu_pressed` | ![menu_pressed](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/menu_pressed.png) | bg_opa=20% bg_color=#bg_light |
| `menu_separator` |  | bg_opa=0 pad_ver=#pad_tiny |
| `msgbox_backdrop_bg` |  | *(not set)* |
| `keyboard_button` | ![keyboard_button](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/keyboard_button.png) | merge=border_muted bg_color=#bg_light shadow_width=0 radius=#radius |
| `keyboard_button_checked_bg` | ![keyboard_button_checked_bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/keyboard_button_checked_bg.png) | bg_color=#primary bg_opa=100% text_color=#text |
| `list_bg` |  | pad_hor=#pad pad_ver=0 pad_gap=0 clip_corner=true |
| `list_btn` | ![list_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/list_btn.png) | border_width=1 border_color=#border border_side=bottom pad_all=#pad_small pad_column=#pad_small |
| `list_item_grow` |  | transform_width=#pad |
| `tab_bg_focus` |  | outline_pad=-#border_width |
| `tab_btn` | ![tab_btn](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/tab_btn.png) | border_color=#primary border_width=4 border_side=bottom pad_top=4 |
| `led` | ![led](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/led.png) | bg_opa=100% bg_color=0xffffff bg_grad_color=#bg_dark radius=circle shadow_width=15dpx shadow_color=0xffffff shadow_spread=5dpx |
| `scale` |  | line_color=#text line_width=2dpx arc_color=#text arc_width=2dpx length=6dpx |
| `slider` | ![slider](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/slider.png) | height=20 bg_color=#highlight bg_opa=100% radius=#radius |
| `slider_indic` |  | radius=#radius |
| `slider_knob` | ![slider_knob](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/slider_knob.png) | radius=#radius border_color=#primary_muted border_width=#border_width |
| `sidebar` |  | *(not set)* |
| `sidebar_btn` |  | *(not set)* |
| `console_btn_anim` |  | recolor=#secondary |
| `estop` | ![estop](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/estop.png) | bg_color=0xf44336 text_color=0xffffff radius=circle |
| `file` | ![file](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/file.png) | merge=btn merge=bg_light merge=border |
| `folder` | ![folder](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/folder.png) | copy=btn |
| `unhomed` |  | bg_color=0xfb9514 border_width=0 |
| `temperature_bar` | ![temperature_bar](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/temperature_bar.png) | bg_opa=100% bg_color=0x2196f3 bg_grad_color=0xf44336 bg_grad_dir=hor |
| `temperature_bar_indic` | ![temperature_bar_indic](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/temperature_bar_indic.png) | bg_color=#border bg_opa=100% |
| `tool_list` |  | *(not set)* |
| `tool_list_tool` | ![tool_list_tool](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/tool_list_tool.png) | copy=card |
| `tool_list_heater` |  | pad_all=0 |
| `tool_selected` | ![tool_selected](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/tool_selected.png) | outline_color=#primary outline_opa=100% outline_width=3 shadow_color=rgb_darken(#primary, 10) shadow_opa=50% shadow_width=0 shadow_offset_x=-#shadow_offset shadow_offset_y=#shadow_offset |
| `tab_button` |  | *(not set)* |
| `slider_buttons` |  | width=40 height=40 |
| `list_header` |  | *(not set)* |
| `list_title` | ![list_title](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/list_title.png) | merge=text_emphasis |
| `graph_legend` |  | min_width=150 |
| `bar_label_bg` | ![bar_label_bg](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/bar_label_bg.png) | bg_color=#bg_light bg_opa=50% radius=#radius pad_hor=#pad |
| `bar_label` |  | *(not set)* |
| `object_cancel_base` | ![object_cancel_base](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/object_cancel_base.png) | copy=card bg_color=#bg_light bg_opa=50% |
| `object_cancel_current` | ![object_cancel_current](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/object_cancel_current.png) | bg_color=#primary bg_opa=100% |
| `object_cancel_cancelled` | ![object_cancel_cancelled](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/object_cancel_cancelled.png) | bg_color=#error bg_opa=100% |
| `debug_borders` | ![debug_borders](../tests/ref_imgs/xml_ui_api/themes/duetscreen/styles/debug_borders.png) | outline_color=#text outline_width=1 outline_opa=100% |

### Colours

Palette colours, used as attribute values, e.g. `style_bg_color="#primary"`:

`#bg_dark`, `#bg`, `#bg_light`, `#primary`, `#primary_muted`, `#secondary`, `#secondary_muted`, `#text`, `#text_muted`, `#text_header`, `#border`, `#border_muted`, `#highlight`, `#shadow`, `#success`, `#warning`, `#error`

![The palette](../tests/ref_imgs/xml_ui_api/themes/duetscreen/colours.png)

### Consts

Used as attribute values, e.g. `style_radius="#radius"`.

| Const | Value in `base` |
| --- | --- |
| `#border_width` | 2 |
| `#outline_width` | 3 |
| `#radius` | 5 |
| `#pad_large` | 10 |
| `#pad` | 5 |
| `#pad_small` | 2 |
| `#pad_tiny` | 1 |
| `#shadow_offset` | 3 |
| `#transition_time` | 80 |

### Fonts

Used as `style_text_font="…"`: `font_header`, `font_normal`, `font_emphasis`, `font_subdued`

![The fonts](../tests/ref_imgs/xml_ui_api/themes/duetscreen/fonts.png)

### Images

Used as image sources, e.g. `style_bg_image_src="…"`: `symbol_ok`


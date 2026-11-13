# XML UI

DuetScreen builds its UI from XML files loaded at runtime. Everything the screen shows is XML, from the home screen ([home.xml](../assets/ui/components/home/home.xml)) down; C++ provides the widgets XML is built from, the data, and the presenters that act on it. The user can rearrange the home screen on the screen itself, choosing widgets for its dashboard, sidebar and status bar ([Layouts and widgets](#layouts-and-widgets)).

The engine is [helix-xml](https://github.com/AndyEveritt/helix-xml) (`libraries/helix-xml`), an MIT fork of the XML engine LVGL shipped before 9.5. The DuetScreen side is [src/UI/Xml](../src/UI/Xml).

**Every name XML can use** (data, actions, presenters, widgets and their attributes, components, styles, colours, consts and fonts) is listed in the [UI API reference](XML_UI_API.md), which is generated from the code.

## Concepts

| Concept | What it is | Where |
| --- | --- | --- |
| Bundle | A directory of XML components with a `manifest.json` | `assets/ui/`, `/boot/ui/<bundle>/` |
| Component | One XML file: a reusable piece of UI, or a whole screen | `assets/ui/**/*.xml` |
| View | Shows a component in C++: `Xml::XmlView` | [View/XmlView.h](../src/UI/Xml/View/XmlView.h) |
| Presenter | C++ behaviour for one component: declares subjects and lists, handles actions | [View/XmlPresenter.h](../src/UI/Xml/View/XmlPresenter.h), `src/UI/Presenters/<Name>/` |
| Data source | Publishes printer data (fans, tools, …) as global subjects and lists | [Data/](../src/UI/Xml/Data) |
| Subject | A named value a widget binds to (`bind_text="name"`) | [Binding/Subjects.h](../src/UI/Xml/Binding/Subjects.h) |
| List | A dynamic-length list of rows; each row has its own subjects, and may have lists of its own | [Binding/List.h](../src/UI/Xml/Binding/List.h) |
| History | A list of named series and their recent values, recorded at a fixed rate, for graphs | [Binding/History.h](../src/UI/Xml/Binding/History.h) |
| Scope | The names a view can use while it is built: subjects and lists | [Binding/Scope.h](../src/UI/Xml/Binding/Scope.h) |
| Action | A named request from XML (`<action name="…">`) handled in C++ | [View/Actions.h](../src/UI/Xml/View/Actions.h) |
| Widget | A DuetScreen XML element backed by C++, e.g. `ds_button`, `ds_list`, `ds_modal`; each registers a spec of its attributes | [Widgets/](../src/UI/Widgets), [reference](XML_UI_API.md#widgets) |
| Page | A view inside a screen, opened by name: `<ds_view name="control" view="control"/>` | [Widgets/DsView.h](../src/UI/Widgets/Layout/DsView.h), [Navigation.h](../src/UI/Core/Navigation.h) |
| Slot | A place whose widgets the user chooses: `<ds_layout slot="dashboard"/>` | [Widgets/DsLayout.h](../src/UI/Widgets/Layout/DsLayout.h) |
| Layout | What a slot shows: which widgets, and where (a grid of cells, or a strip) | [Layout/Layout.h](../src/UI/Xml/Layout/Layout.h), `assets/ui/layouts/` |
| Placeable widget | A component the user can place in a slot, with a widget file saying where it fits, its sizes and its settings | [Layout/WidgetCatalogue.h](../src/UI/Xml/Layout/WidgetCatalogue.h), `widgets/` |
| Theme | An XML file setting the palette, fonts and every style; extends another theme | [UI/Styles/Theme.h](../src/UI/Styles/Theme.h), `assets/ui/themes/` |

A view never contains logic and a presenter never touches a widget. Data flows one way, from C++ to XML through subjects; requests flow the other way, from XML to C++ through actions. Data about the printer is global, published by data sources, so any view (and any widget the user places anywhere) can show it; a presenter holds only its own view's state, such as whether its dialog is open.

```
 Object model ──events──▶ Data sources ──subjects/lists──▶ XML view ──actions──▶ Presenter ──G-code──▶ Duet
                                                               ▲                     │
                                                               └──── subjects ───────┘
```

## Source layout

```
src/UI/
  Core/        Model and its events, Navigation, the presenter base class
  Xml/         the framework
    Runtime/   XmlUi (start, stop, reload), Bundles (manifest, checks), HotReload, Services (G-code, number pad),
               Translations, GlobalActions
    Binding/   Subjects, Scope, Globals, Fields (declaring names), List and Row, History, Surface
    View/      XmlView, XmlPresenter (and the presenter registry), Actions, NumberEntry, ValueSelector
    Api/       ApiReference (everything XML can use, gathered from the running UI), ApiMarkdown (docs/XML_UI_API.md),
               ApiLint (checks every name the components use against the reference), ApiXsd (docs/XML_UI.xsd),
               ApiImages (the reference's pictures, drawn by the UI itself)
    Theme/     ThemeStyles (theme styles and fonts made available to XML)
    Data/      DataSource, DataSources (the list of sources), one file per area: FanSource, ToolSource, HeatSource,
               ConsoleSource, JobSource, MachineSource, MoveSource, NavigationSource, ResponseSource,
               TemperatureSource, and CustomiseSource (the state of customising the screen)
    Layout/    WidgetCatalogue (the widgets the user can place), Layout (layout files, and repairing them),
               LayoutStore (the layout each slot shows, and saving the user's), LayoutEditor (customising)
  Widgets/     every ds_* widget, beside the C++ class behind it; Widgets.cpp registers them
    Buttons/   ds_button, ds_drag_button, ds_password_toggle
    Inputs/    ds_slider, ds_hue_slider, ds_textarea, ds_keyboard, ds_dropdown, ds_toggle
    Display/   ds_bar, ds_bar_marker, ds_graph, ds_heatmap, ds_plot (layer, rect), ds_icon, ds_thumbnail
    Layout/    ds_tabs, ds_list, ds_modal, ds_view, ds_layout
  Presenters/  the presenters and global actions, a folder per part of the UI, mirroring assets/ui/components/
               (Control/Move/MovePresenter.h shows components/control/move/move.xml); Presenters.cpp registers them
  Lvgl/        thin C++ wrappers of LVGL's widgets (generated by scripts/generate_lvgl_wrappers.py)
  Styles/      global styles; Theme, ThemeFile and Palette build them from theme files
docs/XML_UI_API.md, docs/XML_UI.xsd      the UI API reference and its schema, generated by test_xml_ui_api.cpp
docs/XML_UI_THEMES.md                     every built-in theme, generated with them
tests/ref_imgs/xml_ui_api/                their pictures: reference images, like every other screenshot
assets/ui/                                the built-in bundle
  components/<part>/                      each part of the UI (home, dashboard, control/move, settings, …): its page
                                          and the pieces only it uses; mirrors src/UI/Presenters/ (see Naming below)
  components/common/                      small pieces used by several parts (dialogs, value selector, babystepping)
  components/modals/                      the dialogs every part shares: the number pad, speed and extrusion factors,
                                          babystepping, and the question before cancelling the job
  components/customise/                   customising the screen: its toolbar and dialogs, and the edit overlays
  widgets/<purpose>/                      the widgets the user can place, a file each, by what they are for
  layouts/                                the layout of each slot until the user changes it
  themes/                                 the built-in themes
```

## Hosting a view

In XML, a page of a screen is a `<ds_view>`:

```xml
<ds_view name="dashboard" view="dashboard" home="true"/>
<ds_view name="control" view="control"/>
```

`view` names the XML component shown, with its presenter. Every `<ds_view>` is registered with Navigation under its `name`:

- `<action name="nav.open" arg="control"/>` opens it (also `nav.home` and `nav.back`)
- C++ finds it with `findView<Xml::XmlView>("control")`, e.g. the shared `numberpad`
- modals open over the view called `main_window` (`getModalHost()`)

Pages start hidden; `home="true"` makes a page a home screen (shown when nothing else is open) and `shown="true"` shows a part that is always there, such as the status bar.

In C++, an XML component is hosted with `Xml::XmlView`:

```cpp
Xml::XmlView m_fanView{"fans", parent, "fans"};
```

An `XmlView` behaves like any other `LvObj`: it works in tabs and navigation, and its presenter is active, and receives Model events, only while it is shown.

A view can also be given a presenter, which then replaces the one the component names. Its fields shadow the globals, so a component can show other data: the theme gallery test shows `dashboard_tools.xml` with made-up `tools`.

```cpp
Xml::XmlView toolList{"dashboard_tools", parent, "dashboard_tools", std::make_shared<MyTools>()};
```

In XML, `presenter=` does the same with a registered presenter, so one component can be shown in several places with different data or settings (each placement's presenter is registered in [Presenters.cpp](../src/UI/Presenters/Presenters.cpp)):

```xml
<ds_view name="dashboard_jobs" view="file_browser" presenter="dashboard_jobs"/>
```

Views inside another view (its `<ds_view>`s) are rebuilt with it when bundles are reloaded.

## Bundles

A bundle is a directory with a `manifest.json` and component files. A component's name is its file name without `.xml`, whichever folder it is in, so names must be unique within a bundle (a second file with the same name is reported and not loaded). The folders are only for people: organise a bundle however suits it.

```json
{
  "id": "duetscreen",
  "name": "DuetScreen",
  "version": "1.0.0",
  "ui_api": "1.0",
  "components": ["components/**/*.xml"],
  "themes": ["themes/*.xml"],
  "widgets": ["widgets/**/*.json"],
  "layouts": ["layouts/*.json"]
}
```

`widgets` and `layouts` are the widgets a user can place and the layouts of the slots; see [Layouts and widgets](#layouts-and-widgets).

An entry is a file, or a file name with one `*` wildcard (`themes/*.xml`). A `**` folder just before the file name also looks in every folder below, so `components/**/*.xml` is every XML file anywhere under `components/`.

Bundles are loaded in this order. A component, theme or widget in a later bundle replaces the one with the same name from an earlier bundle.

1. **Built-in**: `assets/ui/`, shipped as `/etc/assets/ui/` on the screen.
2. **User bundles**: each sub-directory of `/boot/ui/` on the SD card's boot partition (`var/ui/` in the simulator), in name order.
3. **The user's own**: `/boot/ui/local/`, the layouts the user made on the screen, written by the screen itself.

Everything the user customises is a file in `/boot/ui/`, never in the screen's settings, so copying that folder copies (or backs up) the whole of it.

If a user component is malformed, names an unknown presenter, or comes from a bundle with a different `ui_api` major version, it isn't loaded and the built-in one stays. The problem is logged and shown as a warning message, with the file name and, for XML errors, the line number.

**Safe mode.** While `/boot/ui/` holds a file called `SAFE_MODE`, only the built-in bundle is loaded, and changes made on the screen aren't saved. If a user bundle makes the screen unusable, put an empty `SAFE_MODE` file on the card from a computer, then remove the bundle.

### Example: change how every fan is shown

To override `fan_card` everywhere fans are listed, create `/boot/ui/my-fans/manifest.json`:

```json
{ "id": "my-fans", "ui_api": "1.0", "components": ["fan_card.xml"] }
```

Then add `/boot/ui/my-fans/fan_card.xml`, which here shows the speed as text and has a 50% button:

```xml
<component>
	<view extends="lv_obj" width="100%" height="content" flex_flow="row">
		<style name="card"/>
		<lv_label bind_text="name" flex_grow="1"/>
		<lv_label bind_text="actual" bind_text-fmt="%d%%"/>
		<ds_button text="50%">
			<action trigger="clicked" name="gcode" arg="M106 P{number} S128"/>
		</ds_button>
	</view>
</component>
```

## Writing XML

The syntax is LVGL 9's XML: components, `<view>`, `<style>`, `bind_*`, `bind_flag_if_*` and so on. helix-xml adds `<repeat>`, `<if>` and expressions. See helix-xml's [README](../libraries/helix-xml/README.md) and [BINDINGS.md](../libraries/helix-xml/docs/BINDINGS.md).

Style properties set on a widget use the `style_` prefix, e.g. `style_pad_all="5"` and `style_flex_main_place="center"`. An unknown attribute is only logged, so check the simulator log when something seems to have no effect.

### Names a view can use

A name is looked up from the inside out:

1. the row the widget is in (inside a `<ds_list>`), then the row that row is in, and so on
2. the view's presenter
3. the globals: data published by the data sources

So in a heater row inside a tool card, `name` is the heater's name, `number` is the tool's number (heater rows have no `number`), and `fans` is the global fan list.

The [UI API reference](XML_UI_API.md) lists the names of each kind: the globals by data source, each presenter's, and the fields of every list's rows. Theme styles (`<style name="card"/>`), palette colours and consts (`style_bg_color="#primary"`, `style_radius="#radius"`) and fonts (`font_header`) are listed there too; they all follow theme changes. Translation keys, for `translation_tag="fan.title"` on labels and `ds_button`, are those in `assets/i18n/*.json`.

A string subject can be compared with `bind_*_if_eq`: the sidebar checks the button of the open page with `<bind_state_if_eq subject="nav.screen" state="checked" ref_value="control"/>`.

### Lists

```xml
<ds_list list="tools" item="dashboard_tool" flex_flow="column"/>
```

builds one `dashboard_tool` per row of `tools` (see [dashboard_tools.xml](../assets/ui/components/dashboard/dashboard_tools.xml)). Inside `dashboard_tool`, a list belonging to the row is shown the same way:

```xml
<ds_list list="heaters" item="tool_heater_row"/>
```

Lists add and remove rows at the end, so existing rows keep their scroll position and in-progress edits when the list changes length.

A long list should be windowed: only the rows on screen are built, and rows are built and deleted as it scrolls. Every item then has the same size, set by the list:

```xml
<ds_list list="files" item="file_browser_row" windowed="true" item_height="90"/>
<ds_list list="files" item="file_browser_tile" windowed="true" item_width="150" item_height="170" flow="row_wrap"/>
```

A keyed list shows only the rows whose `key` field is `value`, for a widget about one fan, bed or tool:

```xml
<ds_list list="fans" key="number" value="2" item="fan_card"/>
<ds_list list="beds" key="number" value="$bed" item="dashboard_heater_card"/>
```

Each item is still a row of the list, so its names and actions work as in the full list. The list watches every row's key, so it follows the data: an item appears when its fan does and goes when it goes, and stays right when the rows are refilled (removing fan 1 moves fan 2 into fan 1's row). Key on the item's number rather than `index`, which is only its position. Any field of the rows can be a key, and every row that matches is shown, so `key="state" value="active"` shows every active heater. A keyed list can't be windowed.

### Graphs

A subject holds only the current value. A graph needs the past, so data sources record into a *history*: a list whose rows are series (each with a `name`), and which adds one value to every series at a fixed rate. Sources record whether or not anything shows the history, so a graph built later starts with what came before it.

```xml
<ds_graph history="temperatures" seconds="60" y_min="0" y_max="300" bind_y_max="temperatures.limit"/>
```

shows the last 60 seconds of every heater (see [dashboard.xml](../assets/ui/components/dashboard/dashboard.xml)). The Y axis runs from `y_min` to `y_max`, or to `bind_y_max`'s value if that is higher. Because a history is a list, `<ds_list list="temperatures" …>` can show its series too.

### DuetScreen widgets

These are in addition to the standard `lv_*` widgets: `ds_button`, `ds_slider`, `ds_list`, `ds_modal`, `ds_tabs`, `ds_graph` and more. Each is listed, with every attribute it takes and the events it raises, under [Widgets](XML_UI_API.md#widgets) in the reference. A widget declares them where it registers itself (`registerXml()` in its `Ds*.cpp`), so that is where to add one.

### Dialogs

A dialog is a `<ds_modal>` whose subject says whether it is open, so the presenter opens and closes it by setting a subject and never touches a widget:

```xml
<ds_modal name="file_dialog" bind_open="dialog" open_value="start_print" width="70%" height="content">
    <action trigger="dismissed" name="files.close_dialog"/>
    …
</ds_modal>
```

For a plain question, `confirm_dialog.xml` is ready-made, as in the shared question before cancelling the job (`modals/job_cancel.xml`):

```xml
<confirm_dialog open="confirm" title_tag="status.print_cancel_title" text_tag="status.print_cancel_message"
                confirm="job.cancel" close="job.close_cancel"/>
```

`question_dialog.xml` asks yes or no with a title and text from subjects, over the view it is in:

```xml
<question_dialog open="confirm" title="confirm_title" text="confirm_text" confirm="move.confirm" close="move.close_dialog"/>
```

A value with - and + and a row of presets, such as the extruder's feed distance, is `value_selector.xml`, backed by a `ValueSelector` the presenter owns ([ValueSelector.h](../src/UI/Xml/View/ValueSelector.h)), which also stores the presets and the value.

Numbers are typed on the number pad. There is one, shared by the whole UI: `numberpad.xml`, hosted in the home screen as `<ds_view name="numberpad" view="numberpad"/>`. A presenter asks it for a number with `Services::askNumber({.header, .min, .max, .value, .onConfirm})`, or for digits and dots, such as an IP address, with `Services::askText({...})` ([Services.h](../src/UI/Xml/Runtime/Services.h)); a `ds_slider` with `numberpad="true"` asks it when its value is tapped. It answers one question at a time, and closes when the user confirms, dismisses it, or changes tab.

A dialog that keeps a number pad beside its own controls, as the speed and extrusion factor dialogs (`factors_dialog.xml`) do, places `numberpad_panel.xml` and owns a `NumberEntry` for it ([NumberEntry.h](../src/UI/Xml/View/NumberEntry.h)), as a value selector owns a `ValueSelector`. Any view opens those dialogs with the global actions `factors.speed` and `factors.extrusion`.

A dialog the whole UI shares is hosted once by the home screen, from `modals/`, and opened with a global action, so any view, and any widget the user places, can open it: the number pad, the factor dialogs, babystepping (`babystep.open`) and the question before cancelling the job (`job.ask_cancel`, which cancels only if the user says so).

When the user closes it another way (tapping outside, the back button) it raises `dismissed`, and the presenter clears the subject. It is shown over the main window but belongs to the view it is written in: its actions go to that view's presenter, and it is deleted with that view.

### Actions

```xml
<action trigger="clicked" name="fans.set_speed" arg="100"/>
```

An action is resolved like a name: first the presenter of the nearest enclosing view, then the global actions. A handler can find the rows the widget is inside: `args.row<FanRow>()`, or from a heater row both `args.row<ToolHeaterRow>()` and `args.row<ToolRow>()`.

`trigger` is any LVGL event name (`clicked`, `value_changed`, `long_pressed`, ...), `value_confirmed` (an edit finished) or `dismissed` (a `<ds_modal>` was closed by the user).

`arg` may contain placeholders: `{value}` is the edited value, and any other `{name}` is the current value of that subject, looked up from where the `<action>` is written. In a fan row, `arg="M106 P{number} S{value}"`.

The global actions, and each presenter's, are listed with what they take in the [reference](XML_UI_API.md#global-actions). Global actions are registered with `Actions::addGlobal()` and, like presenters, last for the life of the program; the tool and heater actions are in [HeaterActions.cpp](../src/UI/Presenters/Control/Temperature/HeaterActions.cpp), `factors.speed` and `factors.extrusion`, which open the shared speed and extrusion factor dialogs, in [FactorsPresenter.cpp](../src/UI/Presenters/Modals/FactorsPresenter.cpp), the E-stop's `estop.dragged` in [EStopAction.cpp](../src/UI/Presenters/Sidebar/EStopAction.cpp), the job's `job.pause`, `job.resume` and `job.print_again` in [JobActions.cpp](../src/UI/Presenters/Dashboard/JobActions.cpp), the shared dialogs' in `src/UI/Presenters/Modals/`, and `customise.start` in [CustomisePresenter.cpp](../src/UI/Presenters/Customise/CustomisePresenter.cpp).

## Layouts and widgets

The dashboard, the sidebar and the status bar's buttons are *slots*: places whose widgets the user chooses, on the screen itself. A slot is a `<ds_layout>`:

```xml
<ds_layout slot="sidebar" flex_flow="column" width="100%" height="100%"/>
```

It shows the slot's *layout*, and follows it as it changes, rebuilding only the widgets that changed. The built-in slots and their layouts are listed, with every widget that can be placed and its picture, under [Layouts](XML_UI_API.md#layouts) in the reference.

### Layout files

A layout is a file in a bundle's `layouts/`, listed by the manifest's `layouts`:

```json
{
  "format": 1,
  "slot": "dashboard",
  "kind": "grid",
  "columns": 20, "rows": 12,
  "items": [
    {"id": "tools", "widget": "tools", "x": 0, "y": 0, "w": 12, "h": 6},
    {"id": "fan1", "widget": "fan", "x": 12, "y": 8, "w": 8, "h": 2, "settings": {"fan": 1}}
  ]
}
```

- A `grid` places each widget in a rectangle of cells (20 by 12 unless it says otherwise; the user can change it). A `strip` shows its widgets one after another, in order, sized by their components and placed by the `<ds_layout>`'s own layout, e.g. `flex_flow="column"`.
- An item names a widget, a unique `id`, and the values of the widget's settings it doesn't leave at their defaults.
- An item marked `"locked": true` can be moved but not removed or reconfigured. Only the first bundle's layout of a slot can lock (the built-in bundle's, for the built-in slots), and a locked item missing from a later layout is put back: Settings and the E-stop are always on the sidebar.
- A slot shows the user's own layout (saved by the screen in the local bundle), else the last bundle's. *Reset* on the screen goes back to the last bundle's, so a shared bundle's layout becomes the default.
- A layout is repaired rather than rejected, and each repair reported like any other problem: an overlapping widget moves, a widget's size keeps to its limits and the grid, a setting it can't take goes back to its default, and a widget that isn't installed keeps its place, showing that it is missing.
- `format` is the file's version. An older format is upgraded when it is read; a newer one is rejected.

### Writing a widget

A widget is a component and a widget file, `widgets/<purpose>/<id>.json` (the widget's id is the file's name), listed by the manifest's `widgets`:

```json
{
  "component": "dashboard_fan",
  "title_tag": "widget.fan",
  "description_tag": "widget.fan_desc",
  "category_tag": "widget.category_fans",
  "icon": "fine_tune.png",
  "slots": ["dashboard"],
  "size": {"default": [10, 2], "min": [8, 2]},
  "props": {},
  "settings": [
    {"name": "fan", "kind": "row", "list": "fans", "key": "number", "label": "name", "default": 0,
     "title_tag": "widget.setting_fan"}
  ]
}
```

- `slots` are the slots it can be placed in, and `size` its default, smallest and largest size in a grid, in cells (`[columns, rows]`; with no `max`, up to the whole grid).
- `title`, `description` (or `title_tag`, `description_tag`, translated) and `icon` show it in the widget pool, under its `category` (or `category_tag`).
- `props` are always given to the component; one component can be several widgets, as each sidebar page button is `sidebar_page_button` with its own `page`, `label` and `icon`.
- `settings` are props the user chooses. Each needs a `default`, and a `kind`:

| Kind | The user chooses | Also |
| --- | --- | --- |
| `int` | a number, on the number pad | `min`, `max`, `unit` |
| `bool` | on or off, with a switch | |
| `text` | text, typed in the settings on the keyboard | `max_length` (default 64) |
| `choice` | one of `options` (`[{"value": 30, "title": "30 s"}]`, or `title_tag`) | |
| `row` | a row of a global list, as the printer has them now: which fan, which tool | `list`, `key` (the field stored), `label` (the field shown) |

The component takes them as props, and uses them as `$name`, or `${name}` inside a longer value:

```xml
<component presenter="fans">
	<api>
		<prop name="fan" type="int" default="0"/>
	</api>
	<view extends="lv_obj" width="100%" height="100%" scrollable="false">
		<style name="pad_zero"/>
		<ds_list list="fans" key="number" value="$fan" item="fan_card" width="100%" height="content"/>
	</view>
</component>
```

- The component's root is a plain `lv_obj` (not a `ds_*` widget, which has a C++ object of its own). In a grid it fills its cells (the `<ds_layout>` makes it, so it needn't set a size); in a strip it sizes itself, as the sidebar's buttons take the sidebar's width and share its height (`width="100%" flex_grow="1"`).
- A widget about one of something (a fan, a tool) shows it with a keyed list, so it follows the printer's rows; it shows nothing while the printer has no such one.
- Printer data is global, so a widget shows it wherever it is placed. A widget may name a presenter, and each placed widget then has one of its own.
- The lint checks each widget file (its component takes every prop and setting, a `row` setting's list and fields exist, every translation key exists) and each widget's component with its default settings.

A user bundle adds a widget the same way, and a slot for it to go in if none fits.

### Customising on the screen

*Settings › Display › Customise* starts customising the home screen (the global action `customise.start`). Every placed widget is covered by an edit overlay: a tap selects it, and a drag moves it a cell at a time or along its strip. A tap on an empty cell marks it, and a tap on the mark opens the widget pool to add a widget there. The selected dashboard widget has grab handles on its corners and edges; dragging one moves those edges. A toolbar adds, configures and removes widgets, sets the grid's size, undoes, resets the slot, and ends customising, keeping the changes or throwing them away. Changes show at once; they are saved only at the end, safely (a copy of the old file is kept as `.bak`, and read if the new one can't be).

How it is built:

- [LayoutEditor](../src/UI/Xml/Layout/LayoutEditor.h) holds a draft of every slot and makes each change, checking it (a widget only goes where it fits; locked items stay), with undo. The [layout store](../src/UI/Xml/Layout/LayoutStore.h) shows the drafts, and saves them at the end.
- [`<ds_layout>`](../src/UI/Widgets/Layout/DsLayout.h) puts the edit overlays (XML: `customise_item.xml`, `customise_grid_item.xml`) over its widgets and turns touches into editor calls.
- The edit's state is global data, `customise.*` ([CustomiseSource](../src/UI/Xml/Data/CustomiseSource.h)), which the overlays and the toolbar bind to.
- The toolbar and its dialogs are `customise/customise.xml`, with [CustomisePresenter](../src/UI/Presenters/Customise/CustomisePresenter.h), hosted once by the home screen.

## Themes

A theme is an XML file that sets the palette, the four fonts and the properties of every style. Themes extend each other: the built-in ones all extend [base.xml](../assets/ui/themes/base.xml), which defines every style, and change only what makes them different. A user bundle can add a theme, or replace a built-in one by giving it the same name.

```xml
<theme name="midnight" extends="duetscreen">
	<palette primary_hue="300" chroma="0.08">
		<color name="primary" lightness="+0.1"/>
	</palette>
	<fonts>
		<font name="header" size="20" weight="800" style="bold"/>
	</fonts>
	<consts>
		<const name="radius" value="8"/>
	</consts>
	<styles>
		<style name="card" border_width="0" shadow_width="12" shadow_opa="20%"/>
		<style name="folder" copy="btn"/>
	</styles>
</theme>
```

- **Palette.** Every colour (`bg`, `primary`, `text`, …) is generated from two hues, a chroma and light or dark mode, then the `<color>` adjustments run. See [Palette.h](../src/UI/Styles/Palette.h). `customisable="true"` lets the user pick the hues in the settings.
- **Fonts and consts** are merged down the chain: a theme overrides its parent name by name.
- **Styles** are built in order, the root ancestor's `<style>` elements first. Each element changes one style: `copy="x"` replaces it with style x as it is at that point, `merge="x y"` adds those styles' properties, then the element's own properties are set. So a theme's styles are changes on top of its parent's.
- **Values** are LVGL's XML style values, plus `#name` for a palette colour or const, `7dpx` for DPI-scaled pixels, `lighten(#c, 0.05)` / `darken(#c, 0.05)` and `rgb_darken(#c, 10)`. See [Theme.h](../src/UI/Styles/Theme.h).

Problems in a theme (an unknown style, colour or parent) are reported with the file and line, like component errors.

## Naming

Every name is `snake_case`: components, views, widgets, styles, presenters. Component names are global, so the built-in ones follow these rules to stay unique and say where they belong:

- **A page is named after its part**, which is also the folder it is in: `dashboard`, `control`, `move`, `fans`, `settings`. The `<ds_view>` showing a component has the component's name too (`<ds_view name="move" view="move"/>`), and so does a presenter written for one component (`presenter="move"`). A component shown several ways names each view and presenter after where it is shown: the file browser is `dashboard_jobs`, `dashboard_macros`, `files_jobs` and `files_macros`.
- **Anything else starts with its part's name**, then says what it is: `move_axis`, `dashboard_tool`, `settings_wifi`, `notifications_alert_choice`. A list's item is named for the one thing it shows (`temperature_heater`, `console_line`), or for how it shows it when there is more than one way (`file_browser_row`, `file_browser_tile`).
- **A widget's file is named for the widget** (`widgets/fans/fan.json`, the widget `fan`), in a folder for what it is for (`navigation`, `jobs`, `temperature`, …), not where it goes, as `slots` can name several; its component is named like any other piece of its part (`dashboard_fan`). One component can be several widgets (`page_home`, `page_control`, … are all `sidebar_page_button`). A slot is named for where it is (`dashboard`, `sidebar`, `status_bar`), and so is its layout file.
- **A piece used by several parts is a part of its own**: `file_browser/` is used by the dashboard and the Files page, `status_bar/` and `sidebar/` by the home screen. Small, general pieces go in `common/` with no prefix (`confirm_dialog`, `value_selector`, whose choices are `value_selector_choice`). The dialogs the whole UI shares, hosted once by the home screen, go in `modals/`, each named for what it is (`numberpad`, `factors`) with its pieces prefixed (`numberpad_panel`, `factors_dialog`); their presenters are in `src/UI/Presenters/Modals/`.

Tabs of a page live in their own folder inside it when they are parts in their own right (`control/move/`), or beside the page with its prefix when they are not (`settings/settings_general.xml`).

## Adding a screen

1. Write `assets/ui/components/<part>/<part>.xml`, in a new folder for the part, named as in [Naming](#naming).
2. If it needs printer data that isn't published yet, add it to a data source (or add a source to the list in [DataSources.cpp](../src/UI/Xml/Data/DataSources.cpp)) and describe each in its `fields()` (see below). These names are the public UI API: renaming one breaks user bundles and needs a `UI_API_VERSION_MAJOR` bump in [XmlUi.h](../src/UI/Xml/Runtime/XmlUi.h).
3. If it needs behaviour, write a presenter in the part's folder of `src/UI/Presenters/` (see [FanPresenter](../src/UI/Presenters/Control/Fans/FanPresenter.h)), add it to [Presenters.cpp](../src/UI/Presenters/Presenters.cpp), and name it on the root element: `<component presenter="…">`.
4. Add a `<ds_view>` for it to the screen that shows it (or host it in C++ with `Xml::XmlView`).

### Declaring names

Rows, presenters and data sources all declare their names the same way, once, in `fields()`:

```cpp
void ToolRow::fields(FieldVisitor& f)
{
	f.field("number", number, "Tool number, as used by T");
	f.list("heaters", heaters, "One row per heater of the tool");
}
```

The declaration is used to bind the names into a scope, to publish them globally, and to describe them in the [reference](XML_UI_API.md). Actions are described the same way, where they are registered: `action("fans.set_speed", "Set the fan's speed to …", handler)`.

## Developing

- **Completion in the editor.** [XML_UI.xsd](XML_UI.xsd) is a schema of the DuetScreen elements and every component, generated with the reference. With VS Code's XML extension (redhat.vscode-xml, set up by the devcontainer), component and screen files complete element names, attributes and the names they take (subjects, actions, styles, components, translation keys, …), and show each one's documentation on hover. It offers names but accepts any value, since which are valid depends on where a component is used; the lint checks that. LVGL's own widgets and bindings aren't in it.
- **Hot reload.** The simulator watches the bundle files and rebuilds open views when one changes. Edit `assets/ui/` or `var/ui/` and save.
- **Tests.**
  - [test_xml_ui.cpp](../tests/src/test_cases/UI/test_xml_ui.cpp) covers loading, overrides, per-row and nested-list bindings, actions, reload and themes.
  - [test_layouts.cpp](../tests/src/test_cases/UI/test_layouts.cpp) covers widget and layout files, repairing layouts, saving them safely, and the layout editor; [test_customise.cpp](../tests/src/test_cases/UI/test_customise.cpp) covers `<ds_layout>` and customising the home screen with touches, including adding every widget to every slot it fits.
  - [test_xml_ui_api.cpp](../tests/src/test_cases/UI/test_xml_ui_api.cpp) checks that every action and widget attribute is described, and that [XML_UI_API.md](XML_UI_API.md), [XML_UI_THEMES.md](XML_UI_THEMES.md) and [XML_UI.xsd](XML_UI.xsd) match the code. When one doesn't, the test rewrites the file and fails: review the change in `git diff` and commit it with the code.
    Their pictures are drawn by the UI: in every built-in theme the palette, fonts, a swatch of each style, each widget's example, a sample of LVGL's widgets and the home screen, and each view with sample printer data. Each is a reference image under `tests/ref_imgs/xml_ui_api/`, compared with `EXPECT_EQUAL_PICTURE` like any screenshot, so a changed one is written as `_err` and reviewed with `run_tests.py`; a picture of something that is gone is removed. They show the release firmware, so developer builds don't compare them. To picture a widget, give its spec an `example` and `exampleSubjects` for what the example binds to; the examples are linted too.
    It also lints the built-in bundle ([ApiLint.h](../src/UI/Xml/Api/ApiLint.h)): every subject, list, action, style, `#const`, font, translation key, component and presenter a component uses must exist where the component is used, so a typo fails the test with the file, line and the path of components that led there, e.g. `dashboard_heater.xml:21: no subject `currentx` (in home › dashboard › dashboard_tools › dashboard_tool › dashboard_heater)`.
  - Every theme is checked pixel by pixel by the dashboard and widget screenshots (`home_view/dashboard/theme_*.png`, `theme/*`).
  - Converted screens must still match their existing screenshots in `tests/ref_imgs`.
  - Engine changes belong in helix-xml with tests there (`cmake -S tests -B build` in `libraries/helix-xml`).

## Known limits

- **No float subjects.** `LV_USE_FLOAT` would also switch all of LVGL's drawing maths to float, which existing widgets aren't written for. Use integers (percent, tenths) or preformatted text (heaters publish `current` as text with one decimal place).
- **LVGL 9.5 compatibility shim.** helix-xml targets LVGL 9.6. `libraries/helix-xml-compat` bridges the differences until LVGL is upgraded.
- **Engine warnings don't fail tests.** Unknown attributes are only logged by the engine, and LVGL logging is compiled out of the Test preset; check the simulator log. Unknown *names* (subjects, actions, styles, …) are caught by the lint above.
- **The UI API is the same in every build.** A feature a build lacks keeps its actions and fields, and says so through a field (e.g. `developer_mode`, `shell_available`) that the XML hides its controls with, so the reference and the lint hold for every build.

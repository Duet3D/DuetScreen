# XML UI

DuetScreen can build views from XML files loaded at runtime. The Fan screen is built this way; the rest of the UI is still C++, and both kinds of view work side by side.

The engine is [helix-xml](https://github.com/AndyEveritt/helix-xml) (`libraries/helix-xml`), an MIT fork of the XML engine LVGL shipped before 9.5. The DuetScreen side is [src/UI/Xml](../src/UI/Xml).

## Concepts

| Concept | What it is | Where |
| --- | --- | --- |
| Bundle | A directory of XML components with a `manifest.json` | `assets/ui/`, `/boot/ui/<bundle>/` |
| Component | One XML file: a reusable piece of UI, or a whole screen | `assets/ui/**/*.xml` |
| View | Shows a component in C++: `Xml::XmlView` | [View/XmlView.h](../src/UI/Xml/View/XmlView.h) |
| Presenter | C++ behaviour for one component: declares subjects and lists, handles actions | [View/XmlPresenter.h](../src/UI/Xml/View/XmlPresenter.h), `src/UI/Screens/<Name>/` |
| Data source | Publishes printer data (fans, tools, …) as global subjects and lists | [Data/](../src/UI/Xml/Data) |
| Subject | A named value a widget binds to (`bind_text="name"`) | [Binding/Subjects.h](../src/UI/Xml/Binding/Subjects.h) |
| List | A dynamic-length list of rows; each row has its own subjects, and may have lists of its own | [Binding/List.h](../src/UI/Xml/Binding/List.h) |
| Scope | The names a view can use while it is built: subjects and lists | [Binding/Scope.h](../src/UI/Xml/Binding/Scope.h) |
| Action | A named request from XML (`<action name="…">`) handled in C++ | [View/Actions.h](../src/UI/Xml/View/Actions.h) |
| Widget | A DuetScreen XML element backed by C++: `ds_button`, `ds_slider`, `ds_list` | [Widgets/](../src/UI/Xml/Widgets) |

A view never contains logic and a presenter never touches a widget. Data flows one way, from C++ to XML through subjects; requests flow the other way, from XML to C++ through actions.

```
 Object model ──events──▶ Data sources ──subjects/lists──▶ XML view ──actions──▶ Presenter ──G-code──▶ Duet
                                                               ▲                     │
                                                               └──── subjects ───────┘
```

## Source layout

```
src/UI/Xml/
  Runtime/   XmlUi (start, stop, reload), Bundles (manifest, checks), HotReload, Services (G-code, number pad),
             Translations, GlobalActions
  Binding/   Subjects, Scope, Globals, Fields (declaring names), List and Row
  View/      XmlView, XmlPresenter (and the presenter registry), Actions
  Widgets/   DsButton, DsSlider, DsList
  Theme/     ThemeStyles (theme styles and fonts made available to XML)
  Data/      DataSource, DataSources (the list of sources), one file per area: FanSource, ToolSource, HeatSource
src/UI/Screens/<Name>/<Name>Presenter.*   presenters live with their screen
src/UI/Screens/XmlPresenters.cpp          the list of presenters XML can name
assets/ui/                                the built-in bundle
```

## Hosting a view

```cpp
Xml::XmlView m_fanView{"fan", parent, "fan_screen"};
```

An `XmlView` behaves like any other `LvObj`: it works in tabs and navigation, and its presenter is active, and receives Model events, only while it is shown.

## Bundles

A bundle is a directory with a `manifest.json` and component files. A component's name is its file name without `.xml`.

```json
{
  "id": "duetscreen",
  "name": "DuetScreen",
  "version": "1.0.0",
  "ui_api": "1.0",
  "components": ["components/*.xml", "screens/*.xml"]
}
```

Bundles are loaded in this order. A component in a later bundle replaces the one with the same name from an earlier bundle.

1. **Built-in**: `assets/ui/`, shipped as `/etc/assets/ui/` on the screen.
2. **User bundles**: each sub-directory of `/boot/ui/` on the SD card's boot partition (`var/ui/` in the simulator), in name order.

If a user component is malformed, names an unknown presenter, or comes from a bundle with a different `ui_api` major version, it isn't loaded and the built-in one stays. The problem is logged and shown as a warning message, with the file name and, for XML errors, the line number.

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

| Kind | Names |
| --- | --- |
| Data | The data sources in [Data/](../src/UI/Xml/Data); each header lists its names. `fans`, `tools` (each with `heaters`), `beds`, `chambers`. |
| Theme styles | Every member of `LvglStyles` and `ComponentStyles` by name, e.g. `<style name="card"/>`. They follow theme changes. |
| Fonts | `font_header`, `font_normal`, `font_emphasis`, `font_subdued` |
| Text | `translation_tag="fan.title"` on labels, `translation_tag` on `ds_button`. Keys are those in `assets/i18n/*.json`. |

### Lists

```xml
<ds_list list="tools" item="tool_card" flex_flow="column"/>
```

builds one `tool_card` per row of `tools`. Inside `tool_card`, a list belonging to the row is shown the same way:

```xml
<ds_list list="heaters" item="tool_heater_row"/>
```

Lists add and remove rows at the end, so existing rows keep their scroll position and in-progress edits when the list changes length.

### DuetScreen widgets

These are in addition to the standard `lv_*` widgets.

| Widget | Attributes |
| --- | --- |
| `<ds_button>` | `text`, `translation_tag`, `bind_text`, `icon`, `checkable` |
| `<ds_slider>` | `min`, `max`, `increment`, `decimals`, `label`, `label_tag`, `bind_label`, `bind_value`, `numberpad`. Raises `value_confirmed` when an edit is finished. |
| `<ds_list>` | `list`, `item`: one `item` component per row of the list |

### Actions

```xml
<action trigger="clicked" name="fans.set_speed" arg="100"/>
```

An action is resolved like a name: first the presenter of the nearest enclosing view, then the global actions. A handler can find the rows the widget is inside: `args.row<FanRow>()`, or from a heater row both `args.row<ToolHeaterRow>()` and `args.row<ToolRow>()`.

`trigger` is any LVGL event name (`clicked`, `value_changed`, `long_pressed`, ...) or `value_confirmed`.

`arg` may contain placeholders: `{value}` is the edited value, and any other `{name}` is the current value of that subject, looked up from where the `<action>` is written. In a fan row, `arg="M106 P{number} S{value}"`.

| Global action | `arg` |
| --- | --- |
| `gcode` | G-code to send, e.g. `M220 S{value}` |

## Adding a screen

1. Write `assets/ui/screens/<name>.xml`.
2. If it needs printer data that isn't published yet, add it to a data source (or add a source to the list in [DataSources.cpp](../src/UI/Xml/Data/DataSources.cpp)) and document the names in the source's header. These names are the public UI API: renaming one breaks user bundles and needs a `UI_API_VERSION_MAJOR` bump in [XmlUi.h](../src/UI/Xml/Runtime/XmlUi.h).
3. If it needs behaviour, write a presenter in `src/UI/Screens/<Name>/` (see [FanPresenter](../src/UI/Screens/Fan/FanPresenter.h)), add it to [XmlPresenters.cpp](../src/UI/Screens/XmlPresenters.cpp), and name it on the root element: `<component presenter="…">`.
4. Host it with `Xml::XmlView`.

### Declaring names

Rows, presenters and data sources all declare their names the same way, once, in `fields()`:

```cpp
void ToolRow::fields(FieldVisitor& f)
{
	f.field("number", number, "Tool number, as used by T");
	f.list("heaters", heaters, "One row per heater of the tool");
}
```

The declaration is used to bind the names into a scope, to publish them globally, and to describe them.

## Developing

- **Hot reload.** The simulator watches the bundle files and rebuilds open views when one changes. Edit `assets/ui/` or `var/ui/` and save.
- **Tests.**
  - [test_xml_ui.cpp](../tests/src/test_cases/UI/test_xml_ui.cpp) covers loading, overrides, per-row and nested-list bindings, actions and reload.
  - Converted screens must still match their existing screenshots in `tests/ref_imgs`.
  - Engine changes belong in helix-xml with tests there (`cmake -S tests -B build` in `libraries/helix-xml`).

## Known limits

- **No float subjects.** `LV_USE_FLOAT` would also switch all of LVGL's drawing maths to float, which existing widgets aren't written for. Use integers (percent, tenths) or preformatted text (heaters publish `current` as text with one decimal place).
- **LVGL 9.5 compatibility shim.** helix-xml targets LVGL 9.6. `libraries/helix-xml-compat` bridges the differences until LVGL is upgraded.
- **Engine warnings don't fail tests.** Unknown attributes and unresolved bindings are logged, but LVGL logging is compiled out of the Test preset. Check the simulator log, and add structural assertions in tests.

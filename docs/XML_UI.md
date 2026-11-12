# XML UI

DuetScreen can build views from XML files loaded at runtime. The Fan screen is the first view built this way. The rest of the UI is still C++ and both kinds of view work side by side.

The engine is [helix-xml](https://github.com/AndyEveritt/helix-xml) (`libraries/helix-xml`). It is an MIT fork of the XML engine LVGL shipped before 9.5. The DuetScreen side is in [src/UI/Xml](../src/UI/Xml).

## How a view is put together

| Part | Lives in | Job |
| --- | --- | --- |
| View | `assets/ui/**/*.xml` | Layout and styling. Binds widgets to subjects and raises named actions. No logic. |
| Data | [OmPublisher](../src/UI/Xml/OmPublisher.h) | Mirrors the object model into global subjects and list models, e.g. `om.fans`. |
| Presenter | [src/UI/Xml/Presenters](../src/UI/Xml/Presenters) | Screen-specific state and the actions a view raises, e.g. `fans.set_speed`. Never touches widgets. |

Hosting a view in C++ takes one line:

```cpp
Xml::XmlView m_fanView{"fan", parent, "fan_screen"};
```

An `XmlView` behaves like any other `LvObj`: it works in tabs and navigation, and it activates its presenter when shown.

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
			<action trigger="clicked" name="fans.set_speed" arg="50"/>
		</ds_button>
	</view>
</component>
```

## Writing XML

The syntax is LVGL 9's XML: components, `<view>`, `<style>`, `bind_*`, `bind_flag_if_*` and so on. helix-xml adds `<repeat>`, `<if>` and expressions. See helix-xml's [README](../libraries/helix-xml/README.md) and [BINDINGS.md](../libraries/helix-xml/docs/BINDINGS.md).

Style properties set on a widget use the `style_` prefix, e.g. `style_pad_all="5"` and `style_flex_main_place="center"`. An unknown attribute is only logged, so check the simulator log when something seems to have no effect.

### Available to every view

| Kind | Names |
| --- | --- |
| Theme styles | Every member of `LvglStyles` and `ComponentStyles` by name, e.g. `<style name="card"/>`. They follow theme changes. |
| Fonts | `font_header`, `font_normal`, `font_emphasis`, `font_subdued` |
| Text | `translation_tag="fan.title"` on labels, `translation_tag` on `ds_button`. Keys are those in `assets/i18n/*.json`. |
| Data | See [OmPublisher.h](../src/UI/Xml/OmPublisher.h) for the `om.*` subjects and models. |

### DuetScreen widgets

These are in addition to the standard `lv_*` widgets.

| Widget | Attributes |
| --- | --- |
| `<ds_button>` | `text`, `translation_tag`, `bind_text`, `icon`, `checkable` |
| `<ds_slider>` | `min`, `max`, `increment`, `decimals`, `label`, `label_tag`, `bind_label`, `bind_value`, `numberpad`. Raises `value_confirmed` when an edit is finished. |
| `<model_list>` | `model`, `item`: one `item` component per entry in the list model |

Inside a `<model_list>` item, names resolve against **that item's** subjects first, then the screen's, then globals. So `fan_card` simply says `bind_text="name"`. This uses helix-xml's subject contexts (`lv_xml_create_with_ctx()`).

### Actions

```xml
<action trigger="clicked" name="fans.set_speed" arg="100"/>
```

An action is resolved like a subject: first the presenter of the nearest enclosing view, then the global actions. A handler receives the index of the `<model_list>` item the event came from.

`trigger` is any LVGL event name (`clicked`, `value_changed`, `long_pressed`, ...) or `value_confirmed`.

| Global action | `arg` |
| --- | --- |
| `gcode` | G-code to send. `{value}` is replaced by the edited value and `{index}` by the list item index, e.g. `M220 S{value}`. |

## Adding a screen

1. Write `assets/ui/screens/<name>.xml`.
2. If it needs data the object model publisher doesn't have yet, add it to `OmPublisher` and document the name there. These names are the public UI API: renaming one breaks user bundles, which needs a `UI_API_VERSION_MAJOR` bump in [XmlUi.h](../src/UI/Xml/XmlUi.h).
3. If it needs behaviour, add a presenter in `src/UI/Xml/Presenters/` (see `FansPresenter`), register it in `registerBuiltinPresenters()`, and name it on the root element: `<component presenter="…">`.
4. Host it with `Xml::XmlView`.

## Developing

- **Hot reload.** The simulator watches the bundle files and rebuilds open views when one changes. Edit `assets/ui/` or `var/ui/` and save.
- **Tests.**
  - [test_xml_ui.cpp](../tests/src/test_cases/UI/test_xml_ui.cpp) covers loading, overrides, per-item bindings, actions and reload.
  - Converted screens must still match their existing screenshots in `tests/ref_imgs`.
  - Engine changes belong in helix-xml with tests there (`cmake -S tests -B build` in `libraries/helix-xml`).

## Known limits

- **No float subjects.** `LV_USE_FLOAT` would also switch all of LVGL's drawing maths to float, which existing widgets aren't written for. Use integers (percent, tenths) or strings.
- **LVGL 9.5 compatibility shim.** helix-xml targets LVGL 9.6. `libraries/helix-xml-compat` bridges the differences until LVGL is upgraded.
- **Engine warnings don't fail tests.** Unknown attributes and unresolved bindings are logged, but LVGL logging is compiled out of the Test preset. Check the simulator log, and add structural assertions in tests.

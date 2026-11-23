# UI

The screen is built from XML loaded at runtime ([docs/XML_UI.md](../../docs/XML_UI.md) describes how, and
[docs/XML_UI_API.md](../../docs/XML_UI_API.md) lists everything XML can use). C++ provides what the XML is built from:

- [Core](./Core): the `Model` and its events, `Navigation`, and the presenter base class
- [Xml](./Xml): the framework that loads the XML, binds it to data and runs its actions
- [Widgets](./Widgets): every `ds_*` widget XML can use, beside the C++ class behind it
- [Presenters](./Presenters): the behaviour behind each part of the UI, a folder per part, mirroring
  [assets/ui/components](../../assets/ui/components)
- [Lvgl](./Lvgl): thin C++ wrappers of LVGL's widgets
- [Styles](./Styles): the styles themes are built into

A presenter never sees a widget: it publishes data as subjects, which the XML binds to, and handles the actions the
XML raises. Layout and styling belong in the XML and the themes.

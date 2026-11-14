/*
 * Names.h
 *
 * Every name XML uses for a subject or a list starts with where it comes from, so a reader knows where to look:
 *
 *     row.<field>                  the list row (or <ds_scope> item) the widget is in: its row's fields, or, for an
 *                                  object model row, the item's fields (`row.current`, `row.value|int`)
 *     parent.<field>               the row around that one (`parent.parent.<field>` the one around that, ...)
 *     presenter.<field>            the presenter of the view the widget is in (src/UI/Presenters/)
 *     widget.<field>               the room the widget's place gives it (Layout/PlacementScope.h)
 *     source.<source>.<field>      a data source, in Data/<Source>Source.cpp, e.g. `source.machine.printing`
 *     om.<path>                    the printer's object model (Om/OmBindings.h)
 *
 * Translation keys are written `tr.<key>` (Runtime/Translations.h), and actions `presenter.<action>` or
 * `actions.<group>.<action>` (View/Actions.h). A component's own `<subjects>` and its `$props` are LVGL's.
 *
 * Each name is only looked for where its prefix says: `row.x` in the innermost row, never an outer one, and an
 * unprefixed name is found nowhere, so a name can't be shadowed or picked up from somewhere unexpected.
 */

#pragma once

#include <optional>
#include <string_view>

namespace UI::Xml::Names
{
	inline constexpr std::string_view ROW = "row.";				// The innermost row's fields
	inline constexpr std::string_view PARENT = "parent.";		// The fields of the row around it
	inline constexpr std::string_view PRESENTER = "presenter."; // The view's presenter's fields
	inline constexpr std::string_view WIDGET = "widget.";		// The widget's place's fields
	inline constexpr std::string_view SOURCE = "source.";		// A data source's fields: `source.<source>.<field>`
	inline constexpr std::string_view OM = "om.";				// The object model

	/** `name` without `prefix`, or nothing if it doesn't start with it */
	inline std::optional<std::string_view> after(std::string_view name, std::string_view prefix)
	{
		if (!name.starts_with(prefix))
		{
			return std::nullopt;
		}
		return name.substr(prefix.size());
	}
} // namespace UI::Xml::Names

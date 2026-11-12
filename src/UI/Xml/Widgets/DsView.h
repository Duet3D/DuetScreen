/*
 * DsView.h
 *
 * <ds_view>: a view inside a screen, e.g. the pages of the home screen.
 *
 *     <ds_view name="control" view="control"/>
 *     <ds_view name="dashboard" view="dashboard" home="true"/>
 *
 * `view` names an XML component (shown with its presenter, see View/XmlView.h) or, if there is no such component, a
 * C++ view (see View/NativeViews.h). The view is registered with Navigation as `name` (default: `view`), so it can be
 * opened (`<action name="nav.open" arg="control"/>`) and found (`findView<T>("control")`).
 *
 * A view starts hidden, like any screen that isn't open. `home="true"` makes it a home screen: shown when nothing
 * else is open. `shown="true"` shows it without taking part in navigation, for always-present parts such as a
 * status bar. Every lv_obj attribute works too (size, grid placement, …), except `hidden`, which navigation owns.
 */

#pragma once

#include "helix_xml.h"

namespace UI::Xml::DsView
{
	/** Register `<ds_view>` with the XML engine */
	void registerXml();
} // namespace UI::Xml::DsView

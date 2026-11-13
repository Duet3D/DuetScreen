/*
 * GlobalActions.h
 *
 * Actions every view can raise, whatever its presenter (see View/Actions.h): `gcode`, `nav.open`, `nav.home` and
 * `nav.back`. What each does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::Xml::GlobalActions
{
	/** Register the global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::GlobalActions

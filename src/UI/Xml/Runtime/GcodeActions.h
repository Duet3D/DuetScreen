/*
 * GcodeActions.h
 *
 * The global action that sends G-code, for any view (see View/Actions.h): `actions.gcode.send`. What it does is in its
 * registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::Xml::GcodeActions
{
	/** Register the G-code actions as global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::GcodeActions

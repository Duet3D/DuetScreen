/*
 * DialogActions.h
 *
 * The global action that closes a dialog from inside it, for any view: `actions.dialog.close`, e.g. on a Cancel
 * button. What it does is in its registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::Xml::DialogActions
{
	/** Register the dialog actions as global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::DialogActions

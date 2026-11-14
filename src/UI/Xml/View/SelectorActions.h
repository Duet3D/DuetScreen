/*
 * SelectorActions.h
 *
 * The global actions that change a value selector (ValueSelector.h), for any view: `actions.selector.decrement`,
 * `.increment` and `.edit` name the selector in their arg (its id, e.g. `jog.distance`); `actions.selector.pick` and
 * `.edit_choice` are raised from a row of its `choices`. What each does is in its registration, and in the UI API
 * reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::Xml::SelectorActions
{
	/** Register the selector actions as global actions. Call after Actions::init(). */
	void registerAll();
} // namespace UI::Xml::SelectorActions

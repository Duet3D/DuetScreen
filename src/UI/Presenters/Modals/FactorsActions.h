/*
 * FactorsActions.h
 *
 * The global actions of the shared speed and extrusion factor dialogs (src/UI/Xml/Data/FactorsSource.h), for any
 * view: `actions.factors.speed` and `actions.factors.extrusion` open them, `actions.factors.set` and
 * `actions.factors.close` are raised by the dialogs. What each does is in its registration, and in the UI API
 * reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::FactorsActions
{
	/** Register the factor dialog actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::FactorsActions

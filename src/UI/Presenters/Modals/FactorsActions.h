/*
 * FactorsActions.h
 *
 * The global actions that open the shared speed and extrusion factor dialogs (FactorsPresenter.h), for any view:
 * `actions.factors.speed` and `actions.factors.extrusion`.
 */

#pragma once

namespace UI::FactorsActions
{
	/** Register the factor dialog actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::FactorsActions

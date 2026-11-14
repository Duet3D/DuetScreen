/*
 * BabystepActions.h
 *
 * The global action that opens the shared babystepping dialog (BabystepDialogPresenter.h), for any view:
 * `actions.babystep.open`.
 */

#pragma once

namespace UI::BabystepActions
{
	/** Register the babystepping actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::BabystepActions

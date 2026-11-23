/*
 * CustomiseActions.h
 *
 * The global actions for customising the home screen (CustomisePresenter.h), for any view: `actions.customise.start`
 * and `actions.customise.open_catalogue`.
 */

#pragma once

namespace UI::CustomiseActions
{
	/** Register the customising actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::CustomiseActions

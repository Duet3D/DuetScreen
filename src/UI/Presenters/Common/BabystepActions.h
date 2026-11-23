/*
 * BabystepActions.h
 *
 * Babystepping, for any view (src/UI/Xml/Data/BabystepSource.h): `actions.babystep.up`, `.down` and `.reset` move Z,
 * `actions.babystep.open` opens the shared dialog. What each does is in its registration, and in
 * the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::BabystepActions
{
	/** Register the babystepping actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::BabystepActions

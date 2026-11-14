/*
 * ConsoleActions.h
 *
 * Typing and sending in the console, for any view (src/UI/Xml/Data/ConsoleSource.h): `actions.console.input`,
 * `.send`, `.clear`, `.recall` and `.shell`. Every console input shares what is typed. What each does is in its
 * registration, and in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

namespace UI::ConsoleActions
{
	/** Register the console actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::ConsoleActions

/*
 * SystemActions.h
 *
 * Acting on the screen itself, for any view: `actions.system.restart`, `.reboot`, `.erase_and_restart`,
 * `.clear_cache`, `.upgrade_from_github`, `.send_config`, `.hardware_test` and `.buildroot_setup`. What each does is
 * in its registration, and in the UI API reference (docs/XML_UI_API.md). Facts about the screen are source.system.*
 * (src/UI/Xml/Data/SystemSource.h).
 */

#pragma once

namespace UI::SystemActions
{
	/** Register the system actions as global actions. Call once at startup. */
	void registerAll();
} // namespace UI::SystemActions

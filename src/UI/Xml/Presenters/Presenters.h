/*
 * Presenters.h
 *
 * The presenters XML components can name with `<component presenter="…">`.
 */

#pragma once

namespace UI::Xml
{
	/**
	 * Register every built-in presenter. Called by XmlUi::init(); registration is explicit (not static initialisers)
	 * so the linker can't drop a presenter nothing else references.
	 */
	void registerBuiltinPresenters();
} // namespace UI::Xml

/*
 * Presenters.h
 *
 * What the XML UI can use from C++: the presenters components can name (`<component presenter="…">`), and the global
 * actions every view can raise. Each presenter lives in the folder of the part of the UI it belongs to, which mirrors
 * assets/ui/components/.
 */

#pragma once

namespace UI
{
	/** Register every built-in presenter and global action. Call once, before Xml::init() loads the bundles. */
	void registerPresenters();
} // namespace UI

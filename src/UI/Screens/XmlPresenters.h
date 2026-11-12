/*
 * XmlPresenters.h
 *
 * The presenters XML components can name with `<component presenter="…">`.
 */

#pragma once

namespace UI
{
	/** Register every built-in XML presenter. Call once, before Xml::init() loads the bundles. */
	void registerXmlPresenters();
} // namespace UI

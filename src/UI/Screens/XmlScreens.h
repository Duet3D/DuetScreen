/*
 * XmlScreens.h
 *
 * What the XML UI can use from C++ screens: the presenters components can name (`<component presenter="…">`) and
 * the C++ views XML screens can host (`<ds_view view="…">`) until they are rewritten in XML.
 */

#pragma once

namespace UI
{
	/** Register every built-in presenter and hostable C++ view. Call once, before Xml::init() loads the bundles. */
	void registerXmlScreens();
} // namespace UI

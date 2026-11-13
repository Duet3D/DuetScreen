/*
 * DsView.h
 *
 * <ds_view>: a view inside a screen, e.g. the pages of the home screen. Its attributes are in registerXml().
 *
 * The view is registered with Navigation as its `name`, so C++ can find it with `findView<T>("control")`.
 */

#pragma once

#include "helix_xml.h"

namespace UI::Xml::DsView
{
	/** Register `<ds_view>` with the XML engine */
	void registerXml();
} // namespace UI::Xml::DsView

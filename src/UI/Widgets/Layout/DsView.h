/*
 * DsView.h
 *
 * <ds_view>: a view inside a screen, e.g. the pages of the home screen. Its attributes are in registerXml().
 *
 * The view is registered with Navigation as its `name`, so C++ can find it with `findView<T>("control")`.
 */

#pragma once

#include "helix_xml.h"

namespace UI
{
	class LvObj;
}

namespace UI::Xml::DsView
{
	/** Register `<ds_view>` with the XML engine */
	void registerXml();

	/**
	 * Show `view`, a view just built inside `parent`, as part of whatever it is in: shown (its presenter active) now
	 * if that is on screen, or else when that is shown, e.g. a page in a tab that isn't open
	 */
	void showWithParent(LvObj& view, lv_obj_t* parent);
} // namespace UI::Xml::DsView

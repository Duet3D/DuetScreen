/*
 * NativeViews.h
 *
 * C++ views that XML can host with `<ds_view view="name">`. This is how a screen written in XML contains parts that
 * are still C++, so the UI can move to XML one view at a time: once a view is rewritten as an XML component of the
 * same name, `<ds_view>` picks the component and the registration here can go.
 *
 * Registrations last for the life of the program; the built-in ones are in src/UI/Screens/XmlScreens.cpp.
 */

#pragma once

#include "UI/Components/LVGL/LvObj.h"
#include <concepts>
#include <functional>
#include <string>
#include <string_view>

namespace UI::Xml::NativeViews
{
	/**
	 * Creates a view inside `parent`. Ownership passes to the caller, which deletes the view with its widgets.
	 * @param name Name of the view's root widget
	 */
	using Factory = std::function<LvObj*(const std::string& name, lv_obj_t* parent)>;

	/** Make `factory` available as `<ds_view view="view">`, replacing any earlier one */
	void add(std::string view, Factory factory);

	/** Make view type V, constructed as `V(name, parent)`, available as `<ds_view view="view">` */
	template <typename V>
		requires std::derived_from<V, LvObj> && std::constructible_from<V, const std::string&, LvObj&>
	void add(std::string view)
	{
		add(std::move(view),
			[](const std::string& name, lv_obj_t* parent) -> LvObj*
			{
				LvObjRef parentRef(parent);
				return new V(name, parentRef);
			});
	}

	/** @return whether a view called `view` is registered */
	bool exists(std::string_view view);

	/** A new `view` inside `parent`, or nullptr if there is no such view */
	LvObj* create(std::string_view view, const std::string& name, lv_obj_t* parent);
} // namespace UI::Xml::NativeViews

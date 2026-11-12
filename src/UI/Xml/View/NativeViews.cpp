/*
 * NativeViews.cpp
 */

#include "NativeViews.h"
#include <map>

namespace UI::Xml::NativeViews
{
	/** Factories by view name */
	static std::map<std::string, Factory, std::less<>>& registry()
	{
		static std::map<std::string, Factory, std::less<>> s_factories;
		return s_factories;
	}

	void add(std::string view, Factory factory)
	{
		registry()[std::move(view)] = std::move(factory);
	}

	bool exists(std::string_view view)
	{
		return registry().find(view) != registry().end();
	}

	LvObj* create(std::string_view view, const std::string& name, lv_obj_t* parent)
	{
		auto it = registry().find(view);
		return it == registry().end() ? nullptr : it->second(name, parent);
	}
} // namespace UI::Xml::NativeViews

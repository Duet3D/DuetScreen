/*
 * XmlPresenter.cpp
 */

#include "XmlPresenter.h"
#include "Debug.h"
#include <map>

namespace UI::Xml::Presenters
{
	static std::map<std::string, PresenterFactory, std::less<>>& registry()
	{
		static std::map<std::string, PresenterFactory, std::less<>> s_factories;
		return s_factories;
	}

	void add(std::string name, PresenterFactory factory)
	{
		registry()[std::move(name)] = std::move(factory);
	}

	bool exists(std::string_view name)
	{
		return registry().find(name) != registry().end();
	}

	std::shared_ptr<XmlPresenter> create(std::string_view name)
	{
		auto it = registry().find(name);
		if (it == registry().end())
		{
			return nullptr;
		}
		return it->second();
	}

	void clear()
	{
		registry().clear();
	}
} // namespace UI::Xml::Presenters

/*
 * XmlPresenter.cpp
 */

#include "XmlPresenter.h"
#include <map>

namespace UI::Xml::Presenters
{
	/** Factories by presenter name */
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
		return it == registry().end() ? nullptr : it->second();
	}

	std::vector<std::string> names()
	{
		std::vector<std::string> result;
		for (const auto& [name, factory] : registry())
		{
			result.push_back(name);
		}
		return result;
	}
} // namespace UI::Xml::Presenters

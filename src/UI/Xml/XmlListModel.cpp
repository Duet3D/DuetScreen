/*
 * XmlListModel.cpp
 */

#include "XmlListModel.h"
#include "Debug.h"
#include <algorithm>
#include <unordered_map>

namespace UI::Xml
{
	static std::unordered_map<std::string, ListModelBase*>& registry()
	{
		static std::unordered_map<std::string, ListModelBase*> s_models;
		return s_models;
	}

	ListModelBase::ListModelBase(std::string name)
		: m_name(std::move(name))
	{
		auto [it, inserted] = registry().emplace(m_name, this);
		if (!inserted)
		{
			LOG_WARN("List model '{:s}' registered twice; the newest wins", m_name);
			it->second = this;
		}
	}

	ListModelBase::~ListModelBase()
	{
		// ListModel<T> notifies before its items die; this covers any other subclass
		notifyDestroyed();

		auto it = registry().find(m_name);
		if (it != registry().end() && it->second == this)
		{
			registry().erase(it);
		}
	}

	void ListModelBase::addListener(Listener* listener)
	{
		if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
		{
			m_listeners.push_back(listener);
		}
	}

	void ListModelBase::removeListener(Listener* listener)
	{
		std::erase(m_listeners, listener);
	}

	ListModelBase* ListModelBase::find(std::string_view name)
	{
		auto it = registry().find(std::string(name));
		return it == registry().end() ? nullptr : it->second;
	}

	void ListModelBase::notifyResized(size_t oldSize, size_t newSize)
	{
		ZoneScoped;
		// Copy: a listener may remove itself (e.g. its widget is deleted while rebuilding)
		const auto listeners = m_listeners;
		for (Listener* listener : listeners)
		{
			if (std::find(m_listeners.begin(), m_listeners.end(), listener) != m_listeners.end())
			{
				listener->onModelResized(oldSize, newSize);
			}
		}
	}

	void ListModelBase::notifyDestroyed()
	{
		const auto listeners = std::exchange(m_listeners, {});
		for (Listener* listener : listeners)
		{
			listener->onModelDestroyed();
		}
	}
} // namespace UI::Xml

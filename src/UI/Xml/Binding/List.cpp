/*
 * List.cpp
 */

#include "List.h"
#include "Debug.h"
#include <algorithm>

namespace UI::Xml
{
	ListBase::~ListBase()
	{
		// List<R> notifies before its rows die; this covers any other subclass
		notifyDestroyed();
	}

	void ListBase::addListener(Listener* listener)
	{
		if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
		{
			m_listeners.push_back(listener);
		}
	}

	void ListBase::removeListener(Listener* listener)
	{
		std::erase(m_listeners, listener);
	}

	void ListBase::notifyResized(size_t oldSize, size_t newSize)
	{
		ZoneScoped;
		// Copy: a listener may remove itself or another (e.g. a nested list's widget is deleted)
		const auto listeners = m_listeners;
		for (Listener* listener : listeners)
		{
			if (std::find(m_listeners.begin(), m_listeners.end(), listener) != m_listeners.end())
			{
				listener->onListResized(oldSize, newSize);
			}
		}
	}

	void ListBase::notifyDestroyed()
	{
		const auto listeners = std::exchange(m_listeners, {});
		for (Listener* listener : listeners)
		{
			listener->onListDestroyed();
		}
	}
} // namespace UI::Xml

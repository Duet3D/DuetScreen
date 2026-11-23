/*
 * OmWatch.cpp
 */

#include "OmWatch.h"
#include "Debug.h"
#include "OmBindings.h"

namespace UI::Xml
{
	OmWatch::OmWatch(std::vector<std::string> paths, std::function<void()> callback)
	{
		OmBindings* bindings = OmBindings::current();
		if (bindings == nullptr)
		{
			LOG_ERROR("Can't watch the object model: its names aren't running");
			return;
		}
		m_id = bindings->watch(std::move(paths), std::move(callback));
	}

	OmWatch::~OmWatch()
	{
		stop();
	}

	OmWatch& OmWatch::operator=(OmWatch&& other) noexcept
	{
		if (this != &other)
		{
			stop();
			m_id = other.m_id;
			other.m_id = 0;
		}
		return *this;
	}

	void OmWatch::stop()
	{
		// The object model's names may have gone first, taking their watchers with them
		if (m_id != 0 && OmBindings::current() != nullptr)
		{
			OmBindings::current()->unwatch(m_id);
		}
		m_id = 0;
	}
} // namespace UI::Xml

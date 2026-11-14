/*
 * DataSource.cpp
 */

#include "DataSource.h"
#include "Debug.h"
#include "UI/Xml/Om/OmBindings.h"

namespace UI::Xml
{
	DataSource::~DataSource()
	{
		// The object model's names may have gone first, taking their watchers with them
		if (m_watch != 0 && OmBindings::current() != nullptr)
		{
			OmBindings::current()->unwatch(m_watch);
		}
	}

	void DataSource::watchObjectModel(std::vector<std::string> paths)
	{
		OmBindings* bindings = OmBindings::current();
		if (bindings == nullptr)
		{
			LOG_ERROR("Data source '{:s}' can't watch the object model: its names aren't running", m_name);
			return;
		}
		if (m_watch != 0)
		{
			bindings->unwatch(m_watch);
		}
		m_watch = bindings->watch(std::move(paths), [this] { update(); });
	}
} // namespace UI::Xml

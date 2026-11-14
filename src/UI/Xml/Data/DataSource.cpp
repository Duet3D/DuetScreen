/*
 * DataSource.cpp
 */

#include "DataSource.h"

namespace UI::Xml
{
	void DataSource::watchObjectModel(std::vector<std::string> paths)
	{
		m_watch = OmWatch(std::move(paths), [this] { update(); });
	}
} // namespace UI::Xml

/*
 * ChangeSet.cpp
 */

#include "ChangeSet.h"
#include "Path.h"

namespace OM
{
	void ChangeSet::add(std::string path)
	{
		m_paths.insert(std::move(path));
	}

	bool ChangeSet::touches(std::string_view path) const
	{
		for (const auto& changed : m_paths)
		{
			if (isAtOrBelow(changed, path) || isAtOrBelow(path, changed))
			{
				return true;
			}
		}
		return false;
	}

	void ChangeSet::merge(const ChangeSet& other)
	{
		m_paths.insert(other.m_paths.begin(), other.m_paths.end());
	}
} // namespace OM

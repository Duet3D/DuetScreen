/*
 * ChangeSet.h
 *
 * The paths of the object model that changed, as canonical text (see Path::str()). A recorded path stands for itself
 * and everything below it: if `heat.heaters[1]` is recorded, `heat.heaters[1].current` may have changed too. An array
 * whose length changed is recorded itself, so anything that depends on the length (or on items that came or went)
 * finds it.
 */

#pragma once

#include <set>
#include <string>
#include <string_view>

namespace OM
{
	class ChangeSet
	{
	  public:
		/** Note that `path` (and everything below it) changed */
		void add(std::string path);

		/** Whether nothing changed */
		bool empty() const { return m_paths.empty(); }

		/** Whether something at, below or above `path` changed: the node at `path` may differ */
		bool touches(std::string_view path) const;

		/** The changed paths, sorted, so a path comes just before the paths below it */
		const std::set<std::string, std::less<>>& paths() const { return m_paths; }

		/** Add every path of `other` */
		void merge(const ChangeSet& other);

		/** Forget every change */
		void clear() { m_paths.clear(); }

	  private:
		std::set<std::string, std::less<>> m_paths; // Changed paths
	};
} // namespace OM

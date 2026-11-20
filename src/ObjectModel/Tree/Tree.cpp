/*
 * Tree.cpp
 */

#include "Tree.h"
#include "LockWrapper.h"
#include <algorithm>
#include <fmt/format.h>
#include <string>
#include <utility>
#include <vector>

namespace OM
{
	namespace
	{
		/** Merges one patch into the tree, noting what changes */
		class Merger
		{
		  public:
			/**
			 * @param changes Where changed paths are noted
			 * @param live Whether objects keep the fields the patch doesn't send (a live response)
			 */
			Merger(ChangeSet& changes, bool live)
				: m_changes(changes)
				, m_live(live)
			{
			}

			/**
			 * The node at `path` below `root`, made if it isn't there. A node on the way that has to change kind (e.g.
			 * null to object) is noted as changed.
			 */
			Node& reach(Node& root, const Path& path)
			{
				Node* node = &root;
				Path at;
				for (const auto& segment : path.segments())
				{
					if (const auto* name = std::get_if<std::string>(&segment))
					{
						if (!node->isObject())
						{
							m_changes.add(at.str());
						}
						node = &node->field(*name);
						at = at.field(*name);
					}
					else
					{
						const size_t index = std::get<size_t>(segment);
						if (!node->isArray() || index >= node->size())
						{
							m_changes.add(at.str());
						}
						node = &node->item(index);
						at = at.index(index);
					}
				}
				return *node;
			}

			/**
			 * Make `dst` hold what `src` holds, noting changes
			 * @param path Where `dst` is
			 * @param noted Whether `path`, or a path above it, is already noted as changed; nothing below a noted path
			 * is noted again
			 */
			void assign(Node& dst, const Node& src, const Path& path, bool noted)
			{
				switch (src.kind())
				{
				case Node::Kind::object:
				{
					if (!dst.isObject())
					{
						note(path, noted);
						dst.makeObject();
					}
					src.forEachField([&](std::string_view name, const Node& child)
									 { assign(dst.field(name), child, path.field(name), noted); });
					if (!m_live)
					{
						std::vector<std::string> gone;
						dst.forEachField(
							[&](std::string_view name, const Node&)
							{
								if (src[name].kind() == Node::Kind::missing)
								{
									gone.emplace_back(name);
								}
							});
						for (const auto& name : gone)
						{
							bool fieldNoted = noted;
							note(path.field(name), fieldNoted);
							dst.removeField(name);
						}
					}
					break;
				}
				case Node::Kind::array:
					if (!dst.isArray())
					{
						note(path, noted);
						dst.makeArray();
					}
					if (dst.size() != src.size())
					{
						note(path, noted);
						dst.resize(src.size());
					}
					for (size_t i = 0; i < src.size(); i++)
					{
						assign(dst.item(i), src[i], path.index(i), noted);
					}
					break;
				default:
					if (dst.kind() != src.kind() || dst.text() != src.text())
					{
						note(path, noted);
						dst.setValue(src.kind(), src.text());
					}
					break;
				}
			}

		  private:
			/** Note `path` as changed unless it, or a path above it, already is */
			void note(const Path& path, bool& noted)
			{
				if (!noted)
				{
					m_changes.add(path.str());
					noted = true;
				}
			}

			ChangeSet& m_changes; // Where changes are noted
			bool m_live;		  // Whether objects keep fields the patch doesn't send
		};
	} // namespace

	Tree& Tree::get()
	{
		static Tree s_tree;
		return s_tree;
	}

	Tree::Tree()
	{
		m_root.makeObject();
	}

	void Tree::apply(const Patch& patch)
	{
		ChangeSet changes; // What this response changed
		{
			std::lock_guard lock(mutexModel);
			Merger merger(changes, patch.live());
			Node& target = merger.reach(m_root, patch.root());
			if (patch.partial())
			{
				// Only the elements sent are replaced; the last part sets the length
				if (!target.isArray())
				{
					changes.add(patch.root().str());
					target.makeArray();
				}
				const size_t end = patch.first() + patch.result().size();
				for (size_t i = patch.first(); i < end; i++)
				{
					if (i >= target.size())
					{
						changes.add(patch.root().str());
					}
					merger.assign(target.item(i), patch.result()[i - patch.first()], patch.root().index(i), false);
				}
				if (patch.last() && target.size() != end)
				{
					changes.add(patch.root().str());
					target.resize(end);
				}
			}
			else
			{
				merger.assign(target, patch.result(), patch.root(), false);
			}
			deriveHeaterLists(changes);
			m_changes.merge(changes);
		}
		notify();
		tellWatchers(changes);
	}

	void Tree::deriveHeaterLists(ChangeSet& changes)
	{
		static constexpr std::pair<std::string_view, std::string_view> LISTS[] = {
			{"bedHeaterMapping", "bedHeaters"},
			{"chamberHeaterMapping", "chamberHeaters"},
		};
		for (const auto& [mappingName, listName] : LISTS)
		{
			const Node& mapping = m_root["heat"][mappingName];
			if (!mapping.isArray())
			{
				continue;
			}
			// Each bed's or chamber's first heater, or -1 for none
			std::vector<int64_t> heaters;
			for (size_t i = 0; i < mapping.size(); ++i)
			{
				heaters.push_back(mapping[i].size() > 0 ? mapping[i][0].integer(-1) : -1);
			}
			const Node& list = m_root["heat"][listName];
			bool same = list.isArray() && list.size() == heaters.size();
			for (size_t i = 0; same && i < heaters.size(); ++i)
			{
				same = list[i].integer(-2) == heaters[i];
			}
			if (same)
			{
				continue;
			}
			Node& filled = m_root.field("heat").field(listName);
			filled.makeArray();
			filled.resize(heaters.size());
			for (size_t i = 0; i < heaters.size(); ++i)
			{
				filled.item(i).setValue(Node::Kind::number, std::to_string(heaters[i]));
			}
			changes.add(fmt::format("heat.{:s}", listName));
		}
	}

	void Tree::clear()
	{
		{
			std::lock_guard lock(mutexModel);
			if (m_root.isObject() && m_root.size() == 0)
			{
				return;
			}
			m_root.setValue(Node::Kind::null, "");
			m_root.makeObject();
			m_changes.add(Path().str());
		}
		notify();
		ChangeSet everything;
		everything.add(Path().str());
		tellWatchers(everything);
	}

	size_t Tree::watch(std::vector<std::string> paths, Listener callback)
	{
		std::lock_guard lock(m_listenersMutex);
		const size_t id = m_nextListenerId++;
		m_watchers.push_back({.id = id, .paths = std::move(paths), .callback = std::move(callback)});
		return id;
	}

	void Tree::unwatch(size_t id)
	{
		std::lock_guard lock(m_listenersMutex);
		std::erase_if(m_watchers, [id](const Watcher& watcher) { return watcher.id == id; });
	}

	void Tree::tellWatchers(const ChangeSet& changes)
	{
		// Called without the model lock, so a watcher may read the tree, and with a copy, so it may (un)watch
		std::vector<Listener> told;
		{
			std::lock_guard lock(m_listenersMutex);
			for (const auto& watcher : m_watchers)
			{
				if (std::any_of(watcher.paths.begin(),
								watcher.paths.end(),
								[&changes](const std::string& path) { return changes.touches(path); }))
				{
					told.push_back(watcher.callback);
				}
			}
		}
		for (const auto& callback : told)
		{
			callback();
		}
	}

	ChangeSet Tree::takeChanges()
	{
		std::lock_guard lock(mutexModel);
		ChangeSet changes = std::move(m_changes);
		m_changes.clear();
		m_notified = false;
		return changes;
	}

	size_t Tree::addListener(Listener listener)
	{
		std::lock_guard lock(m_listenersMutex);
		const size_t id = m_nextListenerId++;
		m_listeners.emplace_back(id, std::move(listener));
		return id;
	}

	void Tree::removeListener(size_t id)
	{
		std::lock_guard lock(m_listenersMutex);
		std::erase_if(m_listeners, [id](const auto& entry) { return entry.first == id; });
	}

	void Tree::notify()
	{
		{
			std::lock_guard lock(mutexModel);
			if (m_changes.empty() || m_notified)
			{
				return;
			}
			m_notified = true;
		}
		// Called without the model lock, so a listener may take its own locks in any order
		std::lock_guard lock(m_listenersMutex);
		for (const auto& [id, listener] : m_listeners)
		{
			listener();
		}
	}
} // namespace OM

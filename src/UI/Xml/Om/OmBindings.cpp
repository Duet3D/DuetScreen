/*
 * OmBindings.cpp
 */

#include "OmBindings.h"
#include "Debug.h"
#include "ObjectModel/Tree/Tree.h"
#include "OmForward.h"
#include "OmName.h"
#include "UI/Core/Model.h"
#include "UI/Xml/Binding/Scope.h"
#include <algorithm>
#include <set>

namespace UI::Xml
{
	Subject* OmBindings::subject(const OM::Path& path, std::string_view converterName, std::string_view argument)
	{
		std::string key = path.str();
		if (!converterName.empty())
		{
			key += '|';
			key += converterName;
			if (!argument.empty())
			{
				key += ':';
				key += argument;
			}
		}
		if (auto it = m_fields.find(key); it != m_fields.end())
		{
			return &it->second->subject();
		}

		const Converter* converter = findConverter(converterName);
		if (converter == nullptr)
		{
			LOG_WARN("Object model name '{:s}': there is no converter '{:s}'", key, converterName);
			return nullptr;
		}
		auto field = std::make_unique<OmField>(path, *converter, std::string(argument));
		{
			MODEL_LOCK();
			field->update(OM::Tree::get().root());
		}
		return &m_fields.emplace(std::move(key), std::move(field)).first->second->subject();
	}

	OmList* OmBindings::list(const OM::Path& path)
	{
		std::string key = path.str();
		if (auto it = m_lists.find(key); it != m_lists.end())
		{
			return it->second.get();
		}
		auto list = std::make_unique<OmList>(*this, path);
		{
			MODEL_LOCK();
			list->refresh(OM::Tree::get().root());
		}
		return m_lists.emplace(std::move(key), std::move(list)).first->second.get();
	}

	void OmBindings::flush()
	{
		ZoneScoped;
		MODEL_LOCK();
		const OM::ChangeSet changes = OM::Tree::get().takeChanges();
		if (changes.empty())
		{
			return;
		}
		const OM::Node& root = OM::Tree::get().root();

		// A field's key starts with its path, so the fields at or below a changed path follow it in key order (with a
		// few that only look alike, e.g. "a[10]" after "a[1]", which are harmlessly updated too)
		for (const auto& path : changes.paths())
		{
			for (auto it = m_fields.lower_bound(path); it != m_fields.end() && it->first.starts_with(path); ++it)
			{
				it->second->update(root);
			}
		}
		updateFieldsAbove(changes, root);

		// A list changes with its array's length and its items, so any change at, below or above it counts. Rows a
		// refresh adds can make lists of their own: the map's iterators survive that.
		for (auto& [path, list] : m_lists)
		{
			if (changes.touches(path))
			{
				list->refresh(root);
			}
		}

		// A watcher may watch or unwatch, so call a copy of those to tell
		std::vector<std::function<void()>> told;
		for (const auto& watcher : m_watchers)
		{
			if (std::any_of(watcher.paths.begin(),
							watcher.paths.end(),
							[&changes](const std::string& path) { return changes.touches(path); }))
			{
				told.push_back(watcher.callback);
			}
		}
		for (const auto& callback : told)
		{
			callback();
		}
	}

	void OmBindings::updateFieldsAbove(const OM::ChangeSet& changes, const OM::Node& root)
	{
		// Every path above a change, once
		std::set<std::string, std::less<>> above;
		for (const auto& changed : changes.paths())
		{
			const auto path = OM::Path::parse(changed);
			for (OM::Path up = path ? path->parent() : OM::Path(); !up.empty(); up = up.parent())
			{
				if (!above.insert(up.str()).second)
				{
					break; // Its ancestors are in already
				}
			}
		}
		// A field's key is its path, then `|` and its converter if it has one
		for (const auto& path : above)
		{
			if (auto it = m_fields.find(path); it != m_fields.end())
			{
				it->second->update(root);
			}
			const std::string converted = path + "|";
			for (auto it = m_fields.lower_bound(converted); it != m_fields.end() && it->first.starts_with(converted);
				 ++it)
			{
				it->second->update(root);
			}
		}
	}

	size_t OmBindings::watch(std::vector<std::string> paths, std::function<void()> callback)
	{
		const size_t id = m_nextWatcher++;
		m_watchers.push_back({.id = id, .paths = std::move(paths), .callback = std::move(callback)});
		return id;
	}

	void OmBindings::unwatch(size_t id)
	{
		std::erase_if(m_watchers, [id](const Watcher& watcher) { return watcher.id == id; });
	}

	void OmBindings::refreshAll()
	{
		ZoneScoped;
		MODEL_LOCK();
		OM::Tree::get().takeChanges();
		const OM::Node& root = OM::Tree::get().root();
		for (auto& [key, field] : m_fields)
		{
			field->update(root);
		}
		for (auto& [path, list] : m_lists)
		{
			list->refresh(root);
		}
		std::vector<std::function<void()>> told;
		for (const auto& watcher : m_watchers)
		{
			told.push_back(watcher.callback);
		}
		for (const auto& callback : told)
		{
			callback();
		}
	}

	Subject* OmBindings::subjectFor(const OmBoundPath& path,
									std::string_view converterName,
									std::string_view argument,
									const lv_xml_subject_ctx_t* origin,
									std::string_view key)
	{
		std::string missing;
		auto boundArgument = OmBoundText::bind(
			argument, [origin](std::string_view name) { return lookupSubject(origin, name); }, missing);
		if (!boundArgument)
		{
			LOG_WARN("Object model name '{:s}': no subject '{:s}' for its converter's argument", key, missing);
			return nullptr;
		}
		if (!path.indirect() && !boundArgument->indirect())
		{
			return subject(*path.resolve(), converterName, argument);
		}
		Scope* owner = Scope::owner(origin);
		if (Subject* kept = owner ? owner->keptSubject(key) : nullptr)
		{
			return kept;
		}
		if (auto it = m_forwards.find(key); owner == nullptr && it != m_forwards.end())
		{
			return it->second.get();
		}
		const Converter* converter = findConverter(converterName);
		if (converter == nullptr)
		{
			LOG_WARN("Object model name '{:s}': there is no converter '{:s}'", key, converterName);
			return nullptr;
		}
		auto forward = std::make_unique<OmForward>(*this, path, *converter, std::move(*boundArgument));
		if (owner)
		{
			return &owner->keep(std::string(key), std::move(forward));
		}
		return (m_forwards[std::string(key)] = std::move(forward)).get();
	}

	ListBase* OmBindings::listFor(const OmBoundPath& path, const lv_xml_subject_ctx_t* origin, std::string_view key)
	{
		if (!path.indirect())
		{
			return list(*path.resolve());
		}
		Scope* owner = Scope::owner(origin);
		if (ListBase* kept = owner ? owner->keptList(key) : nullptr)
		{
			return kept;
		}
		if (auto it = m_forwardLists.find(key); owner == nullptr && it != m_forwardLists.end())
		{
			return it->second.get();
		}
		auto forward = std::make_unique<OmForwardList>(*this, path);
		if (owner)
		{
			return &owner->keep(std::string(key), std::move(forward));
		}
		return (m_forwardLists[std::string(key)] = std::move(forward)).get();
	}

	namespace
	{
		OmBindings* s_current = nullptr; // The running UI's bindings
	} // namespace

	OmBindings* OmBindings::current()
	{
		return s_current;
	}

	void OmBindings::setCurrent(OmBindings* bindings)
	{
		s_current = bindings;
	}

	std::optional<OmBoundPath> OmBindings::bindAbsolute(const lv_xml_subject_ctx_t* origin,
														std::string_view name,
														std::string_view path) const
	{
		const auto parsed = OmTemplate::parse(path);
		if (!parsed)
		{
			LOG_WARN("Object model name '{:s}': '{:s}' isn't a path", name, path);
			return std::nullopt;
		}
		std::string missing;
		auto bound = OmBoundPath::bind(
			*parsed, [origin](std::string_view index) { return lookupSubject(origin, index); }, missing);
		if (!bound)
		{
			LOG_WARN("Object model name '{:s}': no subject '{:s}' for its index", name, missing);
		}
		return bound;
	}

	Subject* OmBindings::findSubject(const lv_xml_subject_ctx_t* origin,
									 std::string_view name,
									 std::string_view written)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || !parsed->absolute)
		{
			return nullptr;
		}
		const auto path = bindAbsolute(origin, name, parsed->path);
		return path ? subjectFor(*path, parsed->converter, parsed->argument, origin, written) : nullptr;
	}

	ListBase* OmBindings::findList(const lv_xml_subject_ctx_t* origin, std::string_view name, std::string_view written)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || !parsed->absolute || !parsed->converter.empty())
		{
			return nullptr;
		}
		const auto path = bindAbsolute(origin, name, parsed->path);
		return path ? listFor(*path, origin, written) : nullptr;
	}
} // namespace UI::Xml

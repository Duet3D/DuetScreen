/*
 * OmBindings.cpp
 */

#include "OmBindings.h"
#include "Debug.h"
#include "ObjectModel/Tree/Tree.h"
#include "OmName.h"
#include "UI/Core/Model.h"

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

	bool OmBindings::has(const OM::Path& path) const
	{
		MODEL_LOCK();
		return OM::Tree::get().root().at(path).kind() != OM::Node::Kind::missing;
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

		// A list changes with its array's length and its items, so any change at, below or above it counts. Rows a
		// refresh adds can make lists of their own: the map's iterators survive that.
		for (auto& [path, list] : m_lists)
		{
			if (changes.touches(path))
			{
				list->refresh(root);
			}
		}
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
	}

	Subject* OmBindings::findSubject(const lv_xml_subject_ctx_t* /* origin */, std::string_view name, bool /* last */)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || !parsed->absolute)
		{
			return nullptr;
		}
		const auto path = OM::Path::parse(parsed->path);
		if (!path)
		{
			LOG_WARN("Object model name '{:s}': '{:s}' isn't a path", name, parsed->path);
			return nullptr;
		}
		return subject(*path, parsed->converter, parsed->argument);
	}

	ListBase* OmBindings::findList(const lv_xml_subject_ctx_t* /* origin */, std::string_view name, bool /* last */)
	{
		const auto parsed = OmName::parse(name);
		if (!parsed || !parsed->absolute || !parsed->converter.empty())
		{
			return nullptr;
		}
		const auto path = OM::Path::parse(parsed->path);
		if (!path)
		{
			LOG_WARN("Object model list '{:s}': '{:s}' isn't a path", name, parsed->path);
			return nullptr;
		}
		return list(*path);
	}
} // namespace UI::Xml

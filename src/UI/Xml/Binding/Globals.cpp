/*
 * Globals.cpp
 */

#include "Globals.h"
#include "Subjects.h"
#include "helix_xml.h"
#include <map>
#include <set>

namespace UI::Xml::Globals
{
	/** Global lists by name */
	static std::map<std::string, ListBase*, std::less<>>& lists()
	{
		static std::map<std::string, ListBase*, std::less<>> s_lists;
		return s_lists;
	}

	/** Names of the subjects registered with the engine, so they can be unregistered */
	static std::set<std::string>& subjectNames()
	{
		static std::set<std::string> s_names;
		return s_names;
	}

	void add(const std::string& name, Subject& subject)
	{
		// Replacing a name the engine already has: drop the old registration first
		if (subjectNames().contains(name))
		{
			lv_xml_unregister_subject(nullptr, name.c_str());
		}
		lv_xml_register_subject(nullptr, name.c_str(), subject.raw());
		subjectNames().insert(name);
	}

	void add(const std::string& name, ListBase& list)
	{
		lists()[name] = &list;
	}

	void remove(const std::string& name)
	{
		lists().erase(name);
		if (subjectNames().erase(name) > 0)
		{
			lv_xml_unregister_subject(nullptr, name.c_str());
		}
	}

	ListBase* findList(std::string_view name)
	{
		auto it = lists().find(name);
		return it == lists().end() ? nullptr : it->second;
	}

	void clear()
	{
		for (const auto& name : subjectNames())
		{
			lv_xml_unregister_subject(nullptr, name.c_str());
		}
		subjectNames().clear();
		lists().clear();
	}
} // namespace UI::Xml::Globals

/*
 * Scope.cpp
 */

#include "Scope.h"
#include "Globals.h"
#include "Subjects.h"

namespace UI::Xml
{
	Scope::Scope(lv_xml_subject_ctx_t* parent)
		: m_ctx(lv_xml_subject_ctx_create(parent))
	{
		lv_xml_subject_ctx_set_user_data(m_ctx, this);
	}

	Scope::~Scope()
	{
		// Retained <repeat>/<if> fragments can keep the context alive; make sure nothing finds this object through it
		lv_xml_subject_ctx_set_user_data(m_ctx, nullptr);
		lv_xml_subject_ctx_delete(m_ctx);
	}

	void Scope::add(const std::string& name, Subject& subject)
	{
		lv_xml_subject_ctx_add(m_ctx, name.c_str(), subject.raw());
	}

	void Scope::add(const std::string& name, ListBase& list)
	{
		m_lists[name] = &list;
	}

	ListBase* Scope::findList(const lv_xml_subject_ctx_t* from, std::string_view name)
	{
		for (const lv_xml_subject_ctx_t* ctx = from; ctx != nullptr; ctx = lv_xml_subject_ctx_get_parent(ctx))
		{
			const Scope* scope = fromContext(ctx);
			if (scope == nullptr)
			{
				continue;
			}
			auto it = scope->m_lists.find(name);
			if (it != scope->m_lists.end())
			{
				return it->second;
			}
		}
		return Globals::findList(name);
	}

	const Scope* Scope::fromContext(const lv_xml_subject_ctx_t* ctx)
	{
		return static_cast<const Scope*>(lv_xml_subject_ctx_get_user_data(ctx));
	}
} // namespace UI::Xml

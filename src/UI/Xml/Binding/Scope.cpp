/*
 * Scope.cpp
 */

#include "Scope.h"
#include "Globals.h"
#include "List.h"
#include "Names.h"
#include "Resolver.h"
#include "Subjects.h"

namespace UI::Xml
{
	Scope::Scope(lv_xml_subject_ctx_t* parent, Kind kind)
		: m_ctx(lv_xml_subject_ctx_create(kind == Kind::row ? pastRows(parent) : parent))
		, m_kind(kind)
	{
		lv_xml_subject_ctx_set_user_data(m_ctx, this);
		if (m_kind == Kind::row)
		{
			m_parentRow = rowOf(parent);
			if (m_parentRow != nullptr)
			{
				lv_xml_subject_ctx_ref(m_parentRow);
			}
			// A row answers `parent.*` names, with or without a resolver of its own
			lv_xml_subject_ctx_set_resolver(m_ctx, resolveCb, this);
		}
	}

	Scope::~Scope()
	{
		// Retained <repeat>/<if> fragments can keep the context alive; make sure nothing finds this object through it
		lv_xml_subject_ctx_set_user_data(m_ctx, nullptr);
		lv_xml_subject_ctx_set_resolver(m_ctx, nullptr, nullptr);
		lv_xml_subject_ctx_delete(m_ctx);
		if (m_parentRow != nullptr)
		{
			lv_xml_subject_ctx_unref(m_parentRow);
		}
		// Newest first, so each goes before anything it observes; lists before subjects, as they observe them
		while (!m_keptListsInOrder.empty())
		{
			m_keptListsInOrder.pop_back();
		}
		while (!m_keptSubjectsInOrder.empty())
		{
			m_keptSubjectsInOrder.pop_back();
		}
	}

	void Scope::add(const std::string& name, Subject& subject)
	{
		lv_xml_subject_ctx_add(m_ctx, name.c_str(), subject.raw());
		m_subjects[name] = &subject;
	}

	void Scope::add(const std::string& name, ListBase& list)
	{
		m_lists[name] = &list;
	}

	void Scope::setResolver(Resolver* resolver, std::string prefix)
	{
		m_resolver = resolver;
		m_resolverPrefix = std::move(prefix);
		if (m_kind != Kind::row)
		{
			lv_xml_subject_ctx_set_resolver(m_ctx, resolver ? resolveCb : nullptr, this);
		}
	}

	namespace
	{
		/** Keep `item` as `key` in `byKey` and `inOrder`, destroying any kept as `key` before */
		template <typename T>
		T& keepInOrder(std::map<std::string, T*, std::less<>>& byKey,
					   std::vector<std::unique_ptr<T>>& inOrder,
					   std::string key,
					   std::unique_ptr<T> item)
		{
			T*& kept = byKey[std::move(key)];
			if (kept != nullptr)
			{
				std::erase_if(inOrder, [old = kept](const std::unique_ptr<T>& made) { return made.get() == old; });
			}
			kept = item.get();
			inOrder.push_back(std::move(item));
			return *kept;
		}
	} // namespace

	Subject& Scope::keep(std::string key, std::unique_ptr<Subject> subject)
	{
		return keepInOrder(m_keptSubjects, m_keptSubjectsInOrder, std::move(key), std::move(subject));
	}

	ListBase& Scope::keep(std::string key, std::unique_ptr<ListBase> list)
	{
		return keepInOrder(m_keptLists, m_keptListsInOrder, std::move(key), std::move(list));
	}

	Subject* Scope::keptSubject(std::string_view key) const
	{
		const auto it = m_keptSubjects.find(key);
		return it == m_keptSubjects.end() ? nullptr : it->second;
	}

	ListBase* Scope::keptList(std::string_view key) const
	{
		const auto it = m_keptLists.find(key);
		return it == m_keptLists.end() ? nullptr : it->second;
	}

	Scope* Scope::owner(const lv_xml_subject_ctx_t* ctx)
	{
		for (; ctx != nullptr; ctx = lv_xml_subject_ctx_get_parent(ctx))
		{
			if (auto* scope = static_cast<Scope*>(lv_xml_subject_ctx_get_user_data(ctx)))
			{
				return scope;
			}
		}
		return nullptr;
	}

	ListBase* Scope::findList(const lv_xml_subject_ctx_t* from, std::string_view name)
	{
		for (const lv_xml_subject_ctx_t* ctx = from; ctx != nullptr; ctx = lv_xml_subject_ctx_get_parent(ctx))
		{
			if (const Scope* scope = fromContext(ctx))
			{
				if (ListBase* list = scope->findListHere(from, name, name))
				{
					return list;
				}
			}
		}
		return Globals::findList(from, name);
	}

	Subject* Scope::findHere(const lv_xml_subject_ctx_t* origin, std::string_view name, std::string_view written) const
	{
		if (m_kind == Kind::row)
		{
			if (const auto outer = Names::after(name, Names::PARENT))
			{
				// `parent.x` is the row around this one's `row.x`; `parent.parent.x` its `parent.x`
				const Scope* row = parentRow();
				const std::string there = outer->starts_with(Names::PARENT)
											  ? std::string(*outer)
											  : std::string(Names::ROW) + std::string(*outer);
				return row ? row->findHere(origin, there, written) : nullptr;
			}
		}
		if (const auto it = m_subjects.find(name); it != m_subjects.end())
		{
			return it->second;
		}
		if (m_resolver != nullptr)
		{
			if (const auto rest = Names::after(name, m_resolverPrefix))
			{
				return m_resolver->findSubject(origin, *rest, written);
			}
		}
		return nullptr;
	}

	ListBase* Scope::findListHere(const lv_xml_subject_ctx_t* origin,
								  std::string_view name,
								  std::string_view written) const
	{
		if (m_kind == Kind::row)
		{
			if (const auto outer = Names::after(name, Names::PARENT))
			{
				const Scope* row = parentRow();
				const std::string there = outer->starts_with(Names::PARENT)
											  ? std::string(*outer)
											  : std::string(Names::ROW) + std::string(*outer);
				return row ? row->findListHere(origin, there, written) : nullptr;
			}
		}
		if (const auto it = m_lists.find(name); it != m_lists.end())
		{
			return it->second;
		}
		if (m_resolver != nullptr)
		{
			if (const auto rest = Names::after(name, m_resolverPrefix))
			{
				return m_resolver->findList(origin, *rest, written);
			}
		}
		return nullptr;
	}

	const Scope* Scope::parentRow() const
	{
		return m_parentRow != nullptr ? fromContext(m_parentRow) : nullptr;
	}

	lv_xml_subject_ctx_t* Scope::pastRows(lv_xml_subject_ctx_t* ctx)
	{
		while (ctx != nullptr)
		{
			const Scope* scope = fromContext(ctx);
			if (scope == nullptr || scope->m_kind != Kind::row)
			{
				break;
			}
			ctx = lv_xml_subject_ctx_get_parent(ctx);
		}
		return ctx;
	}

	lv_xml_subject_ctx_t* Scope::rowOf(lv_xml_subject_ctx_t* ctx)
	{
		for (; ctx != nullptr; ctx = lv_xml_subject_ctx_get_parent(ctx))
		{
			const Scope* scope = fromContext(ctx);
			if (scope != nullptr && scope->m_kind == Kind::row)
			{
				return ctx;
			}
		}
		return nullptr;
	}

	lv_subject_t* Scope::resolveCb(const lv_xml_subject_ctx_t* origin, const char* name, void* userData)
	{
		const auto* scope = static_cast<const Scope*>(userData);
		Subject* subject = scope->findHere(origin, name, name);
		return subject ? subject->raw() : nullptr;
	}

	const Scope* Scope::fromContext(const lv_xml_subject_ctx_t* ctx)
	{
		return static_cast<const Scope*>(lv_xml_subject_ctx_get_user_data(ctx));
	}
} // namespace UI::Xml

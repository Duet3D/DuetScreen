/*
 * Scope.h
 *
 * A scope is the set of names XML can use while a part of a view is being built: subjects, for `bind_*` attributes,
 * and lists, for `<ds_list list="…">`.
 *
 * Scopes nest. Every XmlView has a scope holding its presenter's names, and every row of a `<ds_list>` gets a
 * scope of its own whose parent is the scope the list was built in. A name is looked up in the innermost scope first,
 * then outwards, then in the globals (see Globals.h). So in a heater row inside a tool row, `name` is the heater's
 * name, `number` is the tool's number if heaters have no `number`, and `fans` is the global fan list.
 *
 * A scope can also have a resolver (see Resolver.h), which gives names on demand, e.g. a row of an object model list
 * has a name for every field of its element.
 *
 * Bindings resolve their subject once, when a widget is created, so a scope only has to be *active* while a view is
 * being built. The engine side is helix-xml's subject context (libraries/helix-xml/src/xml/lv_xml_subject_ctx.h); a
 * Scope is the C++ owner of one and adds lists to it.
 */

#pragma once

#include "helix_xml.h"
#include <map>
#include <string>
#include <string_view>

namespace UI::Xml
{
	class ListBase;
	class Resolver;
	class Subject;

	class Scope
	{
	  public:
		/**
		 * Create a scope.
		 * @param parent Context of the scope to search after this one (Scope::context(), or the engine's active
		 *        context while a view is being built), or nullptr for a top-level scope, which is followed only by the
		 *        globals. The link is to the context rather than the Scope object because the context is reference
		 *        counted: it outlives its Scope as long as a child needs it.
		 */
		explicit Scope(lv_xml_subject_ctx_t* parent);

		/** Views built in this scope keep their bindings; only list lookups through it stop working */
		~Scope();

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

		/** Make `subject` available as `name`. It must outlive every widget bound to it. */
		void add(const std::string& name, Subject& subject);

		/** Make `list` available as `name`. It must outlive every `<ds_list>` showing it. */
		void add(const std::string& name, ListBase& list);

		/** Ask `resolver` for names this scope doesn't hold (see Resolver.h); nullptr for none. It must outlive the
		 * scope. */
		void setResolver(Resolver* resolver);

		/** The engine's subject context for this scope, for `lv_xml_create_with_ctx()` */
		lv_xml_subject_ctx_t* context() const { return m_ctx; }

		/**
		 * Find a list by name: in the scope owning `from`, then its parents, then the globals, then (the last chance,
		 * see Resolver.h) the scopes' resolvers again
		 * @param from A context, normally the engine's active one while a view is being built; may be nullptr
		 * @return the list or nullptr
		 */
		static ListBase* findList(const lv_xml_subject_ctx_t* from, std::string_view name);

		/**
		 * The last chance for a subject nobody has (see Resolver.h): the resolvers of the scope owning `from` and its
		 * parents, innermost first
		 * @return the subject or nullptr
		 */
		static Subject* lastChanceSubject(const lv_xml_subject_ctx_t* from, std::string_view name);

	  private:
		/** The Scope that owns `ctx`, or nullptr if it has gone or the context isn't a Scope's */
		static const Scope* fromContext(const lv_xml_subject_ctx_t* ctx);

		/** The engine's resolver callback: asks the Scope's resolver (`userData`) for `name` */
		static lv_subject_t* resolveCb(const lv_xml_subject_ctx_t* origin, const char* name, void* userData);

		lv_xml_subject_ctx_t* m_ctx;						  // Engine context; holds the subjects and the parent link
		std::map<std::string, ListBase*, std::less<>> m_lists; // Lists added to this scope
		Resolver* m_resolver = nullptr;						   // Gives names on demand, or nullptr
	};
} // namespace UI::Xml

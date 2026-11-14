/*
 * Scope.h
 *
 * A scope is the set of names XML can use while a part of a view is being built: subjects, for `bind_*` attributes,
 * and lists, for `<ds_list list="…">`. Every name says where it comes from (see Names.h), and each kind of scope holds
 * names of one kind: a view's scope its presenter's (`presenter.*`), a placed widget's its place's (`widget.*`), and
 * a row of a `<ds_list>` (or a `<ds_scope>`'s item) its row's (`row.*`).
 *
 * Scopes nest: a name is looked for in the innermost scope, then outwards, then in the globals (see Globals.h). Rows
 * are the exception. A row's scope is linked past the rows around it, so `row.x` is only ever the innermost row's,
 * however deep the rows; the row around it is reached on purpose, as `parent.x` (and `parent.parent.x`, ...).
 *
 * A scope can also have a resolver (see Resolver.h), which gives names on demand, e.g. a row of an object model list
 * has a name for every field of its item.
 *
 * Bindings resolve their subject once, when a widget is created, so a scope only has to be *active* while a view is
 * being built. The engine side is helix-xml's subject context (libraries/helix-xml/src/xml/lv_xml_subject_ctx.h); a
 * Scope is the C++ owner of one and adds lists to it.
 */

#pragma once

#include "helix_xml.h"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <string_view>

namespace UI::Xml
{
	class ListBase;
	class Resolver;
	class Subject;

	class Scope
	{
	  public:
		/** What a scope holds */
		enum class Kind
		{
			plain, // A view's or a place's names
			row,   // A row's names (`row.*`); the rows around it are reached as `parent.*`
		};

		/**
		 * Create a scope.
		 * @param parent Context of the scope to search after this one (Scope::context(), or the engine's active
		 *        context while a view is being built), or nullptr for a top-level scope, which is followed only by the
		 *        globals. The link is to the context rather than the Scope object because the context is reference
		 *        counted: it outlives its Scope as long as a child needs it.
		 * @param kind A row's scope is searched after `parent`'s nearest scope that isn't a row's, and asks the row
		 *        `parent` is in (if any) for `parent.*` names
		 */
		explicit Scope(lv_xml_subject_ctx_t* parent, Kind kind = Kind::plain);

		/** Views built in this scope keep their bindings; only list lookups through it stop working */
		~Scope();

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

		/** Make `subject` available as `name`. It must outlive every widget bound to it. */
		void add(const std::string& name, Subject& subject);

		/** Make `list` available as `name`. It must outlive every `<ds_list>` showing it. */
		void add(const std::string& name, ListBase& list);

		/**
		 * Ask `resolver` for names this scope doesn't hold (see Resolver.h); nullptr for none. It must outlive the
		 * scope.
		 * @param prefix Only names starting with it are asked for, without it (`row.` gives `current` for
		 *        `row.current`)
		 */
		void setResolver(Resolver* resolver, std::string prefix = {});

		/**
		 * Keep `subject`, made for widgets built in this scope (e.g. by a resolver), for as long as the scope, which
		 * outlives them; keptSubject(`key`) finds it again
		 * @return the subject
		 */
		Subject& keep(std::string key, std::unique_ptr<Subject> subject);

		/** As keep(), for a list */
		ListBase& keep(std::string key, std::unique_ptr<ListBase> list);

		/** The subject kept as `key`, or nullptr */
		Subject* keptSubject(std::string_view key) const;

		/** The list kept as `key`, or nullptr */
		ListBase* keptList(std::string_view key) const;

		/** The Scope owning `ctx`, or if it has none, the nearest of its parents that has one; nullptr if none has */
		static Scope* owner(const lv_xml_subject_ctx_t* ctx);

		/** The engine's subject context for this scope, for `lv_xml_create_with_ctx()` */
		lv_xml_subject_ctx_t* context() const { return m_ctx; }

		/**
		 * Find a list by name: in the scope owning `from`, then its parents, then the globals
		 * @param from A context, normally the engine's active one while a view is being built; may be nullptr
		 * @return the list or nullptr
		 */
		static ListBase* findList(const lv_xml_subject_ctx_t* from, std::string_view name);

	  private:
		/** The Scope that owns `ctx`, or nullptr if it has gone or the context isn't a Scope's */
		static const Scope* fromContext(const lv_xml_subject_ctx_t* ctx);

		/** `ctx`, or if it is a row's, the context its scope is searched after (see Kind::row) */
		static lv_xml_subject_ctx_t* pastRows(lv_xml_subject_ctx_t* ctx);

		/** The innermost row `ctx` is in: its own scope's, or one of its parents'; nullptr if none */
		static lv_xml_subject_ctx_t* rowOf(lv_xml_subject_ctx_t* ctx);

		/**
		 * The subject called `name` in this scope alone: its own, then its resolver's; for a row, a `parent.*` name is
		 * the row around it's
		 * @param written The name as written, for the resolver (see Resolver.h)
		 */
		Subject* findHere(const lv_xml_subject_ctx_t* origin, std::string_view name, std::string_view written) const;

		/** As findHere(), for a list */
		ListBase* findListHere(const lv_xml_subject_ctx_t* origin,
							   std::string_view name,
							   std::string_view written) const;

		/** The row around this one, a row, if it is still there; else nullptr */
		const Scope* parentRow() const;

		/** The engine's resolver callback: findHere() on the Scope (`userData`) */
		static lv_subject_t* resolveCb(const lv_xml_subject_ctx_t* origin, const char* name, void* userData);

		lv_xml_subject_ctx_t* m_ctx;						  // Engine context; holds the subjects and the parent link
		Kind m_kind;										  // What it holds
		lv_xml_subject_ctx_t* m_parentRow = nullptr;		  // For a row, the row around it (a reference), or nullptr
		std::map<std::string, Subject*, std::less<>> m_subjects; // Subjects added to this scope
		std::map<std::string, ListBase*, std::less<>> m_lists;	  // Lists added to this scope
		Resolver* m_resolver = nullptr;						  // Gives names on demand, or nullptr
		std::string m_resolverPrefix;						  // What the names the resolver gives start with
		// Made for this scope, by key, and owned in the order made: a subject or list made later may observe one made
		// earlier (`om.spindles[{row.spindle}]` observes `row.spindle`), so they are destroyed newest first
		std::map<std::string, Subject*, std::less<>> m_keptSubjects;
		std::map<std::string, ListBase*, std::less<>> m_keptLists;
		std::vector<std::unique_ptr<Subject>> m_keptSubjectsInOrder;
		std::vector<std::unique_ptr<ListBase>> m_keptListsInOrder;
	};
} // namespace UI::Xml

/*
 * Resolver.h
 *
 * A resolver gives names to XML on demand, for data whose names aren't a fixed list: the object model has a name for
 * every field it holds (`om.heat.heaters[1].current`), and a list row of the object model has one for every field of
 * its element (`current`). A scope (see Scope.h) or the globals (see Globals.h) can have one.
 *
 * Lookups ask a resolver twice:
 *   1. in order, with `last` false: a scope's resolver right after the scope's own names, the global one after the
 *      global names. A resolver answers names it knows are its own.
 *   2. if nobody had the name, again with `last` true, innermost scope first: a resolver answers names that could be
 *      its own but it can't tell yet (e.g. a field the printer hasn't sent).
 *
 * Whatever a resolver returns must outlive every widget and `<ds_list>` bound to it, and the same name must give the
 * same subject or list each time.
 */

#pragma once

#include "helix_xml.h"
#include <string_view>

namespace UI::Xml
{
	class ListBase;
	class Subject;

	class Resolver
	{
	  public:
		virtual ~Resolver() = default;

		/**
		 * The subject called `name`, or nullptr
		 * @param origin The scope's context the lookup started in (a name can depend on where it is written); may be
		 *        nullptr
		 * @param last Whether this is the last chance: nothing else has the name (see the file comment)
		 */
		virtual Subject* findSubject(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last) = 0;

		/** The list called `name`, or nullptr; as findSubject() */
		virtual ListBase* findList(const lv_xml_subject_ctx_t* origin, std::string_view name, bool last) = 0;
	};
} // namespace UI::Xml

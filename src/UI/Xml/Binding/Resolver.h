/*
 * Resolver.h
 *
 * A resolver gives names to XML on demand, for data whose names aren't a fixed list: the object model has a name for
 * every field it holds (`om.heat.heaters[1].current`), and a list row of the object model has one for every field of
 * its item (`row.current`). A scope (see Scope.h) or the globals (see Globals.h) can have one, and it owns every name
 * after its prefix (see Names.h), whether or not the printer has sent that field yet.
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
		 * @param name The name, without the prefix the resolver was given with (Scope::setResolver()), e.g. `current`
		 *        for `row.current`
		 * @param written The name as written in XML, e.g. `parent.current`: different for each subject a resolver makes
		 *        in one origin, so it can key what it keeps there (Scope::keep())
		 */
		virtual Subject* findSubject(const lv_xml_subject_ctx_t* origin,
									 std::string_view name,
									 std::string_view written) = 0;

		/** The list called `name`, or nullptr; as findSubject() */
		virtual ListBase* findList(const lv_xml_subject_ctx_t* origin,
								   std::string_view name,
								   std::string_view written) = 0;
	};
} // namespace UI::Xml

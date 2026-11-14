/*
 * Globals.h
 *
 * Names every view can use, whatever scope it is built in: the printer data published by the data sources (see
 * Data/DataSource.h), e.g. the `source.jog.distances` list. They are searched after every scope.
 *
 * After the global names come constants (see Constants.h): `100`, `'text'` and `tr.<key>` bind anywhere. The globals
 * can also have a resolver (see Resolver.h), asked last, which gives names on demand: the object model's `om.*` names.
 */

#pragma once

#include "helix_xml.h"
#include <string>
#include <string_view>

namespace UI::Xml
{
	class ListBase;
	class Resolver;
	class Subject;

	namespace Globals
	{
		/** Make `subject` available to every view as `name`. It must stay alive until removed. */
		void add(const std::string& name, Subject& subject);

		/** Make `list` available to every view as `name`. It must stay alive until removed. */
		void add(const std::string& name, ListBase& list);

		/** Remove a global subject or list. Widgets already bound to it keep their binding. */
		void remove(const std::string& name);

		/**
		 * The global list called `name`, else the global resolver's (see the file comment), or nullptr
		 * @param from The context the lookup started in, for the resolver; may be nullptr
		 */
		ListBase* findList(const lv_xml_subject_ctx_t* from, std::string_view name);

		/** Ask `resolver` for names nobody else has (see the file comment); nullptr for none. It must stay alive until
		 * replaced. */
		void setResolver(Resolver* resolver);

		/** Start answering the engine's lookups of names nobody has (after lv_xml_init()) */
		void init();

		/** Remove every global name, every constant and the resolver (when the XML engine shuts down) */
		void clear();
	} // namespace Globals
} // namespace UI::Xml

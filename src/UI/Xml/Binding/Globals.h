/*
 * Globals.h
 *
 * Names every view can use, whatever scope it is built in: the printer data published by the data sources (see
 * Data/DataSource.h), e.g. the `fans` list or the `machine.status` subject. They are searched after every scope.
 */

#pragma once

#include <string>
#include <string_view>

namespace UI::Xml
{
	class ListBase;
	class Subject;

	namespace Globals
	{
		/** Make `subject` available to every view as `name`. It must stay alive until removed. */
		void add(const std::string& name, Subject& subject);

		/** Make `list` available to every view as `name`. It must stay alive until removed. */
		void add(const std::string& name, ListBase& list);

		/** Remove a global subject or list. Widgets already bound to it keep their binding. */
		void remove(const std::string& name);

		/** @return the global list called `name`, or nullptr */
		ListBase* findList(std::string_view name);

		/** Remove every global name (when the XML engine shuts down) */
		void clear();
	} // namespace Globals
} // namespace UI::Xml

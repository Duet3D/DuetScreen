/*
 * Fields.h
 *
 * Anything that gives XML names to use (a list row, a presenter, a data source) declares them in one place, a
 * `fields()` method:
 *
 *     void fields(FieldVisitor& f) override
 *     {
 *         f.field("number", number, "Fan number, as used by M106 P");
 *         f.list("heaters", heaters, "The tool's heaters");
 *     }
 *
 * The same declaration is used to add the names to a scope (ScopeBinder), to publish them globally (Data sources)
 * and to describe the UI API (names and descriptions), so a name is only ever written once in C++.
 */

#pragma once

#include "Scope.h"
#include <string_view>

namespace UI::Xml
{
	class ListBase;
	class Subject;

	/** Receives the fields something declares. See the file comment. */
	class FieldVisitor
	{
	  public:
		virtual ~FieldVisitor() = default;

		/** A subject called `name` */
		virtual void field(std::string_view name, Subject& subject, std::string_view description) = 0;

		/** A list called `name`, shown with `<ds_list list="name">` */
		virtual void list(std::string_view name, ListBase& list, std::string_view description) = 0;
	};

	/** Something that declares fields */
	class FieldProvider
	{
	  public:
		virtual ~FieldProvider() = default;

		/** Declare every name XML can use. See the file comment. */
		virtual void fields(FieldVisitor& f) = 0;
	};

	/** Adds the fields it visits to a scope */
	class ScopeBinder : public FieldVisitor
	{
	  public:
		/** Bind into `scope` */
		explicit ScopeBinder(Scope& scope)
			: m_scope(scope)
		{
		}

		/** Add the subject to the scope */
		void field(std::string_view name, Subject& subject, std::string_view) override
		{
			m_scope.add(std::string(name), subject);
		}

		/** Add the list to the scope */
		void list(std::string_view name, ListBase& list, std::string_view) override
		{
			m_scope.add(std::string(name), list);
		}

	  private:
		Scope& m_scope; // Scope the fields are added to
	};

	/** Finds the field called `name` among those it visits */
	class FieldFinder : public FieldVisitor
	{
	  public:
		/** Look for `name` */
		explicit FieldFinder(std::string_view name)
			: m_name(name)
		{
		}

		/** Note the subject if it is the one looked for */
		void field(std::string_view name, Subject& subject, std::string_view) override
		{
			if (name == m_name)
			{
				foundSubject = &subject;
			}
		}

		/** Note the list if it is the one looked for */
		void list(std::string_view name, ListBase& list, std::string_view) override
		{
			if (name == m_name)
			{
				foundList = &list;
			}
		}

		Subject* foundSubject = nullptr; // The subject called `name`, or nullptr
		ListBase* foundList = nullptr;	   // The list called `name`, or nullptr

	  private:
		std::string_view m_name; // Name looked for; must outlive the visit
	};
} // namespace UI::Xml

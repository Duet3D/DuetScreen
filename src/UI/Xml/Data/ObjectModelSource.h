/*
 * ObjectModelSource.h
 *
 * Makes the object model available to XML as `om.*` names (see Om/OmBindings.h): while it lives, the globals' resolver
 * gives those names, and every change to the object model is flushed to their subjects and lists, once per response
 * the printer sends.
 *
 * Its names are given on demand, so fields() declares none; the UI API reference describes them in its Object model
 * section.
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Om/OmBindings.h"

namespace UI::Xml
{
	class ObjectModelSource : public DataSource
	{
	  public:
		/** Read the object model's schema if it hasn't been, and start giving `om.*` names and following the object
		 * model */
		ObjectModelSource();

		/** Stop giving `om.*` names and following the object model */
		~ObjectModelSource() override;

		/** No names to declare: they are given on demand */
		void fields(FieldVisitor& /* f */) override {}

		/** Read everything again */
		void update() override;

		/** Show what the object model holds after a disconnect (it is cleared with the connection) */
		void clear() override;

		/** The `om.*` names, e.g. for tests */
		OmBindings& bindings() { return m_bindings; }

	  protected:
		/** Listen for object model changes */
		void onInit() override;

	  private:
		/** Bring the names up to date with the object model */
		void changed();

		OmBindings m_bindings;		 // The `om.*` names
		size_t m_treeListener = 0;	 // Our listener on the object model, to remove
	};
} // namespace UI::Xml

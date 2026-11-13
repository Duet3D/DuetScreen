/*
 * FanSource.h
 *
 * Publishes the object model's fans.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"

namespace OM
{
	struct Fan;
}

namespace UI::Xml
{
	/** One fan */
	class FanRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		/** Copy `fan` into the subjects */
		void update(const OM::Fan& fan);

		IntSubject number;		 // Fan number, as used by M106 P
		StringSubject<32> name;	 // Display name
		IntSubject requested;	 // Requested speed, percent
		IntSubject actual;		 // Measured speed, percent
		IntSubject rpm{-1};		 // Tachometer reading, -1 if none
		IntSubject thermostatic; // 1 if temperature controlled
	};

	/** Publishes the `fans` list */
	class FanSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		FanSource();

		/** Declare the `fans` list */
		void fields(FieldVisitor& f) override;

		/** Re-read every fan */
		void update() override;

		/** Empty the list */
		void clear() override;

	  protected:
		/** Listen for fan changes */
		void onInit() override;

	  private:
		List<FanRow> m_fans; // One row per fan
	};
} // namespace UI::Xml

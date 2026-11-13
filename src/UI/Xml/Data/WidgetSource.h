/*
 * WidgetSource.h
 *
 * Publishes a placed widget's names (widget.width_class, …; see UI/Xml/Layout/PlacementScope.h) as globals, for views
 * that aren't placed widgets, such as pages: they have a page's room, so every class is expanded. A placed widget's
 * own names are found before these, so a piece shown both on a page and in a widget (a fan's card, a heater's row)
 * can fit itself to the room it has in either.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Layout/PlacementScope.h"

namespace UI::Xml
{
	class WidgetSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		WidgetSource();

		/** Declare the names, as a placed widget has them */
		void fields(FieldVisitor& f) override;

		/** Nothing to read: the names never change */
		void update() override {}

		/** Nothing to clear */
		void clear() override {}

	  private:
		PlacementScope m_page{PlacementScope::SizeClass::Expanded}; // A page's room: never measured, all expanded
	};
} // namespace UI::Xml

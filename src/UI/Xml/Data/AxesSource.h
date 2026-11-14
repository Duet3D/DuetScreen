/*
 * AxesSource.h
 *
 * Publishes the axes as the controls that jog, home and set them show them, so every view of them shares one: the
 * Move tab, its pieces placed as widgets, and the axes widget. Every axis the user sees is a row of `all`, with its
 * position in the displayed unit (degrees for a rotary axis), whether it is homed and what can be done with it now,
 * allowing for the printer being disconnected or busy, so a view only binds. X and Y also have names of their own,
 * for the XY pad, and every other axis is a row of `columns`, each jogged in a column of its own. The global actions
 * `actions.move.*` act on them (src/UI/Presenters/Control/Move/MoveActions.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI::Xml
{
	/** One axis the user sees */
	class AxisRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<4> letter;	// E.g. "U"
		StringSubject<24> position; // In the displayed unit, e.g. "150.00", or "90.00°" for a rotary axis
		IntSubject homed;			// 1 when homed
		IntSubject jogUsable;		// 1 if it can be jogged
		IntSubject homeUsable;		// 1 if it can be homed
	};

	/** Publishes `source.axes.*` */
	class AxesSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		AxesSource();

		/** Declare `source.axes.*` */
		void fields(FieldVisitor& f) override;

		/** Show every axis' position and what can be done with it */
		void update() override;

		/** Nothing can be done while disconnected */
		void clear() override;

	  protected:
		/** Follow the axes and the printer's state */
		void onInit() override;

	  private:
		/** What can be done with X or Y, for the XY pad */
		struct NamedAxis
		{
			IntSubject homed;	  // 1 when homed
			IntSubject jogUsable; // 1 if it can be jogged
		};

		NamedAxis m_x;					  // `x.*`
		NamedAxis m_y;					  // `y.*`
		List<AxisRow> m_all;			  // `all`
		IntSubject m_allHomed;			  // `all_homed`
		IntSubject m_homeAllUsable;		  // `home_all_usable`
		IntSubject m_disableMotorsUsable; // `disable_motors_usable`
		List<AxisRow> m_columns;		  // `columns`
	};
} // namespace UI::Xml

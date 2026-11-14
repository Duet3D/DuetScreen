/*
 * AxesSource.h
 *
 * Publishes the axes as the controls that jog, home and set them show them, so every view of them shares one: the
 * Move tab, and its X/Y and Z cards placed as widgets. X, Y and Z have names of their own, the other axes the user
 * sees are a list. Each says its position in the displayed unit, whether it is homed and what can be done with it
 * now, allowing for the printer being disconnected or busy, so a view only binds. The global actions `actions.move.*`
 * act on them (src/UI/Presenters/Control/Move/MoveActions.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"

namespace UI::Xml
{
	/** One axis other than X, Y and Z */
	class AxisRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<4> letter;  // E.g. "U"
		StringSubject<24> label;  // E.g. "U: 0.00"
		IntSubject homed;		  // 1 when homed
		IntSubject jogUsable;	  // 1 if it can be jogged
		IntSubject homeUsable;	  // 1 if it can be homed
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
		/** The controls for X, Y or Z */
		struct NamedAxis
		{
			StringSubject<24> label; // E.g. "X: 0.00"
			IntSubject homed;		 // 1 when homed
			IntSubject jogUsable;	 // 1 if it can be jogged
			IntSubject homeUsable;	 // 1 if it can be homed
		};

		NamedAxis m_x;					 // `x.*`
		NamedAxis m_y;					 // `y.*`
		NamedAxis m_z;					 // `z.*`
		IntSubject m_xyHomed;			 // `xy.homed`
		IntSubject m_xyHomeUsable;		 // `xy.home_usable`
		IntSubject m_homeAllUsable;		 // `home_all_usable`
		IntSubject m_disableMotorsUsable; // `disable_motors_usable`
		List<AxisRow> m_others;			 // `others`
		IntSubject m_othersShown;		 // `others_shown`
	};
} // namespace UI::Xml

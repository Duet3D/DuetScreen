/*
 * AxesSource.h
 *
 * Publishes the axes as the controls that jog, home and set them show them, so every view of them shares one: the
 * Move tab, its pieces placed as widgets, and the axes widget. Every axis the user sees is a row of `all`, with its
 * position in the displayed unit (degrees for a rotary axis), whether it is homed and what can be done with it now,
 * allowing for the printer being disconnected or busy, so a view only binds. X, Y and Z also have names of their own,
 * for the XY pad and the Z column. Every other axis is a row of `others`, and one of them is chosen (`other.*`) to be
 * jogged in a single column, so a machine with many axes still shows only one more column. The chosen axis is shared
 * by every view, and lasts until the screen restarts. The global actions `actions.move.*` act on them
 * (src/UI/Presenters/Control/Move/MoveActions.h).
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <optional>

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
		IntSubject chosen;			// In `others`: 1 for the axis chosen to be jogged
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

		/** The axis other than X, Y and Z chosen to be jogged, if there is one and the data sources are running */
		static std::optional<char> chosenAxis();

		/** Choose the axis other than X, Y and Z to be jogged: `letter`, if the printer has it */
		static void choose(char letter);

	  protected:
		/** Follow the axes and the printer's state */
		void onInit() override;

	  private:
		/** What can be done with X, Y or Z, for the XY pad and the Z column */
		struct NamedAxis
		{
			IntSubject homed;	  // 1 when homed
			IntSubject jogUsable; // 1 if it can be jogged
		};

		NamedAxis m_x;					  // `x.*`
		NamedAxis m_y;					  // `y.*`
		NamedAxis m_z;					  // `z.*`
		List<AxisRow> m_all;			  // `all`
		IntSubject m_allHomed;			  // `all_homed`
		IntSubject m_homeAllUsable;		  // `home_all_usable`
		IntSubject m_disableMotorsUsable; // `disable_motors_usable`
		List<AxisRow> m_others;			  // `others`
		IntSubject m_othersShown;		  // `others_shown`
		char m_chosen = '\0';			  // The axis chosen from `others`; the first while it isn't one of them
		StringSubject<4> m_otherLetter;	  // `other.letter`
		IntSubject m_otherHomed;		  // `other.homed`
		IntSubject m_otherJogUsable;	  // `other.jog_usable`
	};
} // namespace UI::Xml

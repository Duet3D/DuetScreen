/*
 * MovePresenter.h
 *
 * Behaviour for the Move tab (assets/ui/screens/move.xml): jog and home each axis, set an axis' position, disable the
 * motors, and choose the distance and speed of a jog.
 *
 * Every control's `*_usable` field already allows for the printer being disconnected or busy, so the view only binds.
 *
 * Fields
 *   connected                1 while connected
 *   x.label, y.label, z.label  e.g. "X: 0.00", in the displayed unit
 *   x.homed, y.homed, z.homed  1 when homed
 *   x.jog_usable, …          1 if the axis can be jogged or set (homed, or moves before homing allowed)
 *   x.home_usable, …         1 if the axis can be homed
 *   xy.homed, xy.home_usable  both X and Y
 *   home_all_usable, disable_motors_usable
 *   axes                     list of the other axes, each with letter, label, homed, jog_usable, home_usable
 *   axes_shown               1 if there are other axes
 *   distance_title, feedrate_title  e.g. "Move Distance (mm)"
 *   distances, feedrates     lists of the jog choices, each with `text` and `selected`
 *   confirm, confirm_title, confirm_text   the question asked before homing or disabling the motors
 *
 * Actions
 *   move.jog          arg "X+", "Z-" … or, in an axes row, "+" or "-": jog by the chosen distance at the chosen speed
 *   move.home         arg "X", "Y", "Z", "XY" or "all", or in an axes row: home (asking first if confirmations are on)
 *   move.disable_motors
 *   move.set_position arg the axis letter, or in an axes row: ask for a position and move there
 *   move.distance, move.feedrate             (in a row) choose it
 *   move.edit_distance, move.edit_feedrate   (in a row) ask for a new value for it
 *   move.confirm, move.close_dialog          answer the question
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"
#include <functional>
#include <string>
#include <vector>

namespace UI
{
	/** One axis' controls */
	class MoveAxisRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<4> letter;	// E.g. "U"
		Xml::StringSubject<24> label;	// E.g. "U: 0.00"
		Xml::IntSubject homed;			// 1 when homed
		Xml::IntSubject jogUsable;		// 1 if it can be jogged
		Xml::IntSubject homeUsable;		// 1 if it can be homed
	};

	/** One jog distance or speed */
	class MoveChoiceRow : public Xml::Row
	{
	  public:
		/** Declare the row's fields */
		void fields(Xml::FieldVisitor& f) override;

		Xml::StringSubject<16> text; // E.g. "0.1"
		Xml::IntSubject selected;	 // 1 for the one in use
	};

	class MovePresenter : public Xml::XmlPresenter
	{
	  public:
		/** Create the presenter */
		MovePresenter();

		/** Declare the Move tab's fields */
		void fields(Xml::FieldVisitor& f) override;

	  protected:
		/** Listen for axes and state */
		void onInit() override;

		/** Show the axes as they are now */
		void onActivate() override;

		/** Disable everything */
		void onDisconnect() override;

	  private:
		/** The controls for X, Y or Z */
		struct NamedAxis
		{
			Xml::StringSubject<24> label; // E.g. "X: 0.00"
			Xml::IntSubject homed;		  // 1 when homed
			Xml::IntSubject jogUsable;	  // 1 if it can be jogged
			Xml::IntSubject homeUsable;	  // 1 if it can be homed
		};

		/** Show every axis' position and what can be done with it */
		void showAxes();

		/** Show the distances and speeds, with the chosen one selected */
		void showChoices();

		/** Do `action`, asking first with `title` and `text` if confirmations are on */
		void confirmThen(std::string title, std::string text, std::function<void()> action);

		/** Home the axis called `letter` */
		static void home(char letter);

		/** Jog the axis called `letter` by the chosen distance, forwards or backwards */
		void jog(char letter, bool forward);

		/** Ask for a position for the axis called `letter`, from the widget `asker`, and move there */
		void askPosition(char letter, lv_obj_t* asker);

		/** The axis letter an action is about: its `arg`'s first character, or its axes row's */
		static char axisOf(const Xml::ActionArgs& args);

		std::vector<float> m_distances;	   // Jog distances, as stored
		std::vector<uint32_t> m_feedrates; // Jog speeds
		size_t m_distance = 4;			   // Index of the chosen distance
		size_t m_feedrate = 3;			   // Index of the chosen speed
		std::function<void()> m_confirmed; // What the question's Yes does

		Xml::IntSubject m_connected;				 // 1 while connected
		NamedAxis m_x;								 // X's controls
		NamedAxis m_y;								 // Y's controls
		NamedAxis m_z;								 // Z's controls
		Xml::IntSubject m_xyHomed;					 // 1 when X and Y are homed
		Xml::IntSubject m_xyHomeUsable;				 // 1 if X and Y can be homed
		Xml::IntSubject m_homeAllUsable;			 // 1 if all axes can be homed
		Xml::IntSubject m_disableMotorsUsable;		 // 1 if the motors can be disabled
		Xml::List<MoveAxisRow> m_axes;				 // The other axes
		Xml::IntSubject m_axesShown;				 // 1 if there are other axes
		Xml::StringSubject<48> m_distanceTitle;		 // Distance row title
		Xml::StringSubject<48> m_feedrateTitle;		 // Speed row title
		Xml::List<MoveChoiceRow> m_distanceChoices;	 // Jog distances
		Xml::List<MoveChoiceRow> m_feedrateChoices;	 // Jog speeds
		Xml::IntSubject m_confirm;					 // 1 while asking
		Xml::StringSubject<64> m_confirmTitle;		 // Question title
		Xml::StringSubject<128> m_confirmText;		 // Question
	};
} // namespace UI

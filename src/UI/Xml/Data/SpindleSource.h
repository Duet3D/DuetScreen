/*
 * SpindleSource.h
 *
 * Publishes the spindle of the tool in use, as the Tools tab's panel shows it: whether it turns and how fast, the speed
 * it starts at and in which direction, and quick speeds. Any tool may have a spindle, a printer's too. The direction a
 * spindle starts in is chosen on the screen, and kept for each spindle until the screen restarts; while it turns, it
 * is the direction it turns in. The global actions `actions.spindle.*` (src/UI/Presenters/Control/Temperature/
 * SpindleActions.h) start it, stop it and choose.
 *
 * The names it publishes are declared in fields(), and listed in the UI API reference (docs/XML_UI_API.md).
 */

#pragma once

#include "DataSource.h"
#include "UI/Xml/Binding/List.h"
#include "UI/Xml/Binding/Subjects.h"
#include <map>
#include <optional>
#include <string>

namespace UI::Xml
{
	class SpindleSource;

	/** A quick speed of the spindle */
	class SpindlePresetRow : public Row
	{
	  public:
		/** Declare the row's fields */
		void fields(FieldVisitor& f) override;

		StringSubject<16> text; // E.g. "12 000"
		IntSubject chosen;		// 1 if the tool starts its spindle at it
		int32_t rpm = 0;		// The speed; not shown
	};

	/** Publishes `source.spindle.*` */
	class SpindleSource : public DataSource
	{
	  public:
		/** Create the source. Nothing is published until DataSources::start(). */
		SpindleSource();

		/** Declare the spindle's fields */
		void fields(FieldVisitor& f) override;

		/** Show the tool in use's spindle */
		void update() override;

		/** Show no spindle */
		void clear() override;

		/** Start the tool in use's spindle at its speed, in the direction chosen (M3 or M4) */
		void start();

		/** Choose the direction the spindle starts in; while it turns, turn it that way now */
		void choose(bool reverse);

		/** Start the tool in use's spindle at `rpm` from now on (M568 F) */
		void setSpeed(int32_t rpm);

		/** `rpm` as shown, its thousands apart, e.g. "12 000" */
		static std::string rpmText(int64_t rpm);

	  protected:
		/** Follow the tools, the tool in use and the spindles */
		void onInit() override;

	  private:
		/** The tool in use and its spindle's number, if it has one (the model lock held) */
		struct InUse
		{
			size_t tool;	// The tool's number
			size_t spindle; // Its spindle's number
		};
		static std::optional<InUse> inUse();

		/** Whether spindle `spindle` starts in reverse: the way it turns, else the way chosen (the model lock held) */
		bool reverse(size_t spindle) const;

		std::map<size_t, bool> m_reverse; // The direction chosen for each spindle, by number; forward if none

		IntSubject m_shown;				   // `shown`
		StringSubject<32> m_title;		   // `title`
		IntSubject m_running;			   // `running`
		StringSubject<32> m_state;		   // `state`
		StringSubject<16> m_current;	   // `current`
		StringSubject<64> m_summary;	   // `summary`
		IntSubject m_reverseChosen;		   // `reverse`
		IntSubject m_canReverse;		   // `can_reverse`
		List<SpindlePresetRow> m_presets;  // `presets`
		IntSubject m_presetsShown;		   // `presets_shown`
	};
} // namespace UI::Xml

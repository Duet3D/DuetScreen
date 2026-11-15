/*
 * JobStatusPresenter.h
 *
 * Behaviour for the job status (assets/ui/components/dashboard/dashboard_job_status.xml): where the job's thumbnail
 * goes. Above the job's name, as large as the room the rest of the card leaves, where that room is at least as tall as
 * the place beside the name (a 56 px square); else beside the name. The thumbnail beside the name is no larger than
 * that square, so the larger place is the taller one, whatever the image's shape (a very wide image goes beside the
 * name a little sooner than it need).
 *
 * The card reports its layout (`job_status.layout`, on `layout_changed`); the room is its content height less what
 * its children that don't grow take. With the thumbnail beside the name, the name's line is taller, so the room
 * measured is a little less than above it would have: it moves back above only with room to spare, and never flips.
 *
 * Its names and actions are declared in fields() and its constructor, and listed in the UI API reference
 * (docs/XML_UI_API.md). Everything else the job status shows is global (source.job.*, actions.job.*).
 */

#pragma once

#include "UI/Xml/Binding/Subjects.h"
#include "UI/Xml/View/XmlPresenter.h"

namespace UI
{
	class JobStatusPresenter : public Xml::XmlPresenter
	{
	  public:
		/** Where the thumbnail goes */
		enum class Place
		{
			None = 0,	// There is no thumbnail
			Above = 1,	// Above the name, as large as the room left
			Beside = 2, // Beside the name, in a small square
		};

		/** The side of the square beside the name, px (as dashboard_job_status.xml sizes it) */
		static constexpr int32_t BESIDE_SIZE = 56;

		/** Create the presenter */
		JobStatusPresenter();

		/** Declare `thumbnail_place` */
		void fields(Xml::FieldVisitor& f) override;

	  private:
		/** The room the card's growing child gets: its content height less what its other shown children take */
		static int32_t roomLeft(lv_obj_t* card);

		Xml::IntSubject m_place{static_cast<int32_t>(Place::None)}; // `thumbnail_place`
	};
} // namespace UI

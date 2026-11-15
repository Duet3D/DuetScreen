/*
 * JobStatusPresenter.cpp
 */

#include "JobStatusPresenter.h"

namespace UI
{
	JobStatusPresenter::JobStatusPresenter()
		: XmlPresenter("job_status")
	{
		action("job_status.layout",
			   "The job's card was laid out (`layout_changed`, on the card): place the thumbnail, whose path is `arg`, "
			   "above the name if the room the card's growing child gets is at least as tall as the square beside "
			   "it, else beside it; nowhere if `arg` is empty",
			   [this](const Xml::ActionArgs& args)
			   {
				   Place place = Place::None;
				   if (!args.arg.empty() && args.target != nullptr)
				   {
					   place = roomLeft(args.target) >= BESIDE_SIZE ? Place::Above : Place::Beside;
				   }
				   if (m_place.get() != static_cast<int32_t>(place))
				   {
					   m_place.set(static_cast<int32_t>(place));
				   }
			   });
	}

	void JobStatusPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("thumbnail_place",
				m_place,
				"Where the job's thumbnail goes: 0 nowhere (it has none), 1 above its name, 2 beside it "
				"(presenter.job_status.layout decides)");
	}

	int32_t JobStatusPresenter::roomLeft(lv_obj_t* card)
	{
		int32_t used = 0;  // What the children that don't grow take
		int32_t fixed = 0; // How many of them are shown
		const uint32_t count = lv_obj_get_child_count(card);
		for (uint32_t i = 0; i < count; ++i)
		{
			lv_obj_t* child = lv_obj_get_child(card, static_cast<int32_t>(i));
			// The child that grows is the room itself, shown or not
			if (lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN) || lv_obj_get_style_flex_grow(child, LV_PART_MAIN) != 0)
			{
				continue;
			}
			used += lv_obj_get_height(child);
			++fixed;
		}
		// With the growing child among them, there is a gap after each of the others
		return lv_obj_get_content_height(card) - used - fixed * lv_obj_get_style_pad_row(card, LV_PART_MAIN);
	}
} // namespace UI

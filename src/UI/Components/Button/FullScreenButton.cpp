#include "FullScreenButton.h"

namespace UI
{
	FullScreenButton::FullScreenButton(const std::string& name, LvObj& parent)
		: Button(name, parent, "Full")
	{
		setFlag(LV_OBJ_FLAG_FLOATING, true);
		setAlign(LV_ALIGN_TOP_RIGHT, -8, 8);
		setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		addClickedCallback([this](lv_event_t* e) { onClicked(e); });
	}

	void FullScreenButton::onClicked(lv_event_t* /* e */)
	{
		UI_LOCK();

		lv_obj_t* parent = getParentPtr();
		if (parent == nullptr)
		{
			return;
		}

		lv_obj_t* grandParent = lv_obj_get_parent(parent);
		if (grandParent == nullptr)
		{
			return;
		}

		lv_obj_set_parent(grandParent, getScreenPtr());
		hide();
	}
} // namespace UI
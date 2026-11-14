/*
 * KeyboardRoom.cpp
 */

#include "KeyboardRoom.h"
#include "UI/Xml/Dialogs/DialogHost.h"
#include <algorithm>
#include <optional>

namespace UI::Xml::KeyboardRoom
{
	namespace
	{
		/** The room made, to give back */
		struct Room
		{
			lv_obj_t* padded = nullptr;		   // The container given more padding, or nullptr
			std::optional<int32_t> ownPadding; // Its own bottom padding, if it had one before
			DialogHost* dialog = nullptr;	   // The dialog surface keeping above the keyboard, or nullptr
			lv_obj_t* keyboard = nullptr;	   // The keyboard up
			lv_obj_t* textarea = nullptr;	   // The box it types into
		};

		Room s_room; // The room made now

		/** The padded container is going: there is nothing to give back to it */
		void paddedDeletedCb(lv_event_t* /* e */)
		{
			s_room.padded = nullptr;
		}

		/** The nearest container around `obj` that scrolls vertically, or nullptr */
		lv_obj_t* scrollingAround(lv_obj_t* obj)
		{
			for (lv_obj_t* parent = lv_obj_get_parent(obj); parent != nullptr; parent = lv_obj_get_parent(parent))
			{
				if (lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE) && (lv_obj_get_scroll_dir(parent) & LV_DIR_VER))
				{
					return parent;
				}
			}
			return nullptr;
		}
	} // namespace

	void make(lv_obj_t* keyboard, lv_obj_t* textarea)
	{
		lv_obj_update_layout(keyboard);
		lv_area_t keys;
		lv_obj_get_coords(keyboard, &keys);

		// Handed on to another box where the room is made already, it stays, and only the box is scrolled to
		DialogHost* host = DialogHost::holding(textarea);
		lv_obj_t* container = host == nullptr ? scrollingAround(textarea) : nullptr;
		if (host != nullptr && host == s_room.dialog)
		{
			s_room.keyboard = keyboard;
			s_room.textarea = textarea;
			host->makeRoom(keys.y1, textarea);
			return;
		}
		if (container != nullptr && container == s_room.padded)
		{
			s_room.keyboard = keyboard;
			s_room.textarea = textarea;
			lv_obj_scroll_to_view_recursive(textarea, LV_ANIM_ON);
			return;
		}

		giveBack();
		s_room.keyboard = keyboard;
		s_room.textarea = textarea;
		if (host != nullptr)
		{
			host->makeRoom(keys.y1, textarea);
			s_room.dialog = host;
			return;
		}
		if (container == nullptr)
		{
			return;
		}
		lv_area_t area;
		lv_obj_get_coords(container, &area);
		const int32_t covered = std::min(area.y2, keys.y2) - keys.y1 + 1;
		if (covered > 0)
		{
			s_room.padded = container;
			lv_style_value_t own;
			if (lv_obj_get_local_style_prop(container, LV_STYLE_PAD_BOTTOM, &own, LV_PART_MAIN) == LV_STYLE_RES_FOUND)
			{
				s_room.ownPadding = own.num;
			}
			lv_obj_set_style_pad_bottom(
				container, lv_obj_get_style_pad_bottom(container, LV_PART_MAIN) + covered, LV_PART_MAIN);
			lv_obj_add_event_cb(container, paddedDeletedCb, LV_EVENT_DELETE, nullptr);
		}
		lv_obj_update_layout(container);
		lv_obj_scroll_to_view_recursive(textarea, LV_ANIM_OFF);
	}

	void giveBack()
	{
		if (s_room.padded != nullptr)
		{
			lv_obj_remove_event_cb(s_room.padded, paddedDeletedCb);
			if (s_room.ownPadding)
			{
				lv_obj_set_style_pad_bottom(s_room.padded, *s_room.ownPadding, LV_PART_MAIN);
			}
			else
			{
				lv_obj_remove_local_style_prop(s_room.padded, LV_STYLE_PAD_BOTTOM, LV_PART_MAIN);
			}
		}
		// The surface may have gone with its screen since
		if (s_room.dialog != nullptr && DialogHost::exists(s_room.dialog))
		{
			s_room.dialog->giveRoomBack();
		}
		s_room = {};
	}

	void putAwayFrom(lv_obj_t* obj)
	{
		lv_obj_t* keyboard = s_room.keyboard;
		lv_obj_t* textarea = s_room.textarea;
		if (keyboard == nullptr || textarea == nullptr || lv_keyboard_get_textarea(keyboard) != textarea)
		{
			return;
		}
		for (lv_obj_t* o = textarea; o != nullptr; o = lv_obj_get_parent(o))
		{
			if (o == obj)
			{
				lv_keyboard_set_textarea(keyboard, nullptr);
				lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
				giveBack();
				return;
			}
		}
	}
} // namespace UI::Xml::KeyboardRoom

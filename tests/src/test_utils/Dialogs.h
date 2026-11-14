/*
 * Dialogs.h
 *
 * What the dialog surface of the screen shown shows (src/UI/Xml/Dialogs/DialogHost.h), for tests: the step on top,
 * its title, and its widgets.
 */

#pragma once

#include "UI/Xml/Dialogs/DialogHost.h"
#include <string>
#include <string_view>

namespace TestDialogs
{
	/** The surface of the screen shown */
	inline UI::Xml::DialogHost& host()
	{
		return UI::Xml::DialogHost::of(lv_screen_active());
	}

	/** The content of the step shown, or nullptr if no dialog is */
	inline lv_obj_t* shown()
	{
		return host().shownContent();
	}

	/** The content of the step shown if it is called `name`, else nullptr */
	inline lv_obj_t* shownNamed(const char* name)
	{
		lv_obj_t* content = shown();
		return content != nullptr && std::string(lv_obj_get_name(content) ? lv_obj_get_name(content) : "") == name
				   ? content
				   : nullptr;
	}

	/** The widget called `name` in the step shown or its footer (kept under it, outside it), or nullptr */
	inline lv_obj_t* find(const char* name)
	{
		lv_obj_t* content = shown();
		if (content == nullptr)
		{
			return nullptr;
		}
		lv_obj_t* found = lv_obj_find_by_name(content, name);
		return found != nullptr ? found : lv_obj_find_by_name(lv_obj_find_by_name(host().root(), "footer"), name);
	}

	/** The text of the label called `name` in the step shown, or empty */
	inline std::string text(const char* name)
	{
		lv_obj_t* label = find(name);
		return label != nullptr ? lv_label_get_text(label) : "";
	}

	/** The title the card's header shows */
	inline std::string title()
	{
		lv_obj_t* label = lv_obj_find_by_name(host().root(), "title");
		return label != nullptr ? lv_label_get_text(label) : "";
	}

	/** Tap the widget called `name` in the step shown */
	inline void tap(const char* name)
	{
		if (lv_obj_t* obj = find(name))
		{
			lv_obj_send_event(obj, LV_EVENT_CLICKED, nullptr);
		}
	}

	/** Type `keys` on the number pad shown, a key at a time ('-' is the sign key), without confirming */
	inline void typeOnPad(std::string_view keys)
	{
		for (const char key : keys)
		{
			const std::string name = key == '-' ? "sign" : key == '.' ? "key_dot" : std::string("key_") + key;
			tap(name.c_str());
		}
	}

	/** Type `keys` on the number pad shown, then tap OK */
	inline void enterOnPad(std::string_view keys)
	{
		typeOnPad(keys);
		tap("ok");
	}

	/** Tap a button of the card's header: "back" or "close" */
	inline void tapHeader(const char* name)
	{
		lv_obj_send_event(lv_obj_find_by_name(host().root(), name), LV_EVENT_CLICKED, nullptr);
	}
} // namespace TestDialogs

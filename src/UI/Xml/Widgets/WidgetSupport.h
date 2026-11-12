/*
 * WidgetSupport.h
 *
 * Helpers shared by the C++ widgets made available to XML.
 */

#pragma once

#include "Debug.h"
#include "helix_xml.h"
#include "src/xml/lv_xml_parser.h"
#include "src/xml/lv_xml_utils.h"

namespace UI::Xml::WidgetSupport
{
	/** The value of attribute `name`, or `fallback` if it isn't set */
	inline const char* attrOr(const char** attrs, const char* name, const char* fallback)
	{
		const char* value = lv_xml_get_value_of(attrs, name);
		return value ? value : fallback;
	}

	/** The subject named by attribute `attr` (e.g. `bind_value`), or nullptr if the attribute isn't set or names none */
	inline lv_subject_t* findSubject(lv_xml_parser_state_t* state, const char** attrs, const char* attr)
	{
		const char* name = lv_xml_get_value_of(attrs, attr);
		if (name == nullptr || name[0] == '\0')
		{
			return nullptr;
		}
		lv_subject_t* subject = lv_xml_find_subject(&state->scope, name);
		if (subject == nullptr)
		{
			LOG_WARN("{:s}=\"{:s}\": no such subject", attr, name);
		}
		return subject;
	}

	/** The widget the XML parser is creating children for */
	inline lv_obj_t* parentOf(lv_xml_parser_state_t* state)
	{
		return static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
	}

	/** The widget the XML parser is applying attributes to */
	inline lv_obj_t* itemOf(lv_xml_parser_state_t* state)
	{
		return static_cast<lv_obj_t*>(lv_xml_state_get_item(state));
	}

	/** Delete the heap-allocated `owner` when LVGL deletes `root` */
	template <typename T>
	void deleteWith(lv_obj_t* root, T* owner)
	{
		lv_obj_add_event_cb(
			root, [](lv_event_t* e) { delete static_cast<T*>(lv_event_get_user_data(e)); }, LV_EVENT_DELETE, owner);
	}
} // namespace UI::Xml::WidgetSupport

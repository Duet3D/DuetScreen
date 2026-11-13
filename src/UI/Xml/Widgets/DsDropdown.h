/*
 * DsDropdown.h
 *
 * <ds_dropdown>: an lv_dropdown whose options and text follow subjects.
 *
 *     <ds_dropdown bind_options="filaments" bind_text="filament_text">
 *         <action trigger="value_changed" name="temperature.pick_filament"/>
 *     </ds_dropdown>
 *
 *   bind_options   a string subject holding the options, one per line
 *   bind_text      a string subject shown on the button instead of the chosen option, e.g. what is loaded
 *
 * The option chosen reaches a `value_changed` action as its text (ActionArgs::text()). Takes every lv_dropdown
 * attribute too.
 */

#pragma once

#include "UI/Components/LVGL/LvDropdown.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsDropdown : public LvDropdown
	{
	  public:
		/** Create a dropdown inside `parent` */
		DsDropdown(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_dropdown>` with the XML engine */
		static void registerXml();

	  private:
		/** Pass a temporary C++ wrapper for `parent` to LvDropdown, which only uses it during construction */
		DsDropdown(const std::string& name, LvObjRef&& parent);

		/** `<ds_dropdown>` create callback: create the C++ dropdown, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_dropdown>` apply callback: lv_dropdown attributes, then the bindings */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_options` observer: show the subject's options */
		static void optionsObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** `bind_text` observer: show the subject's text */
		static void textObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		std::string m_text; // The text shown, kept here because lv_dropdown_set_text() doesn't copy it
	};
} // namespace UI::Xml

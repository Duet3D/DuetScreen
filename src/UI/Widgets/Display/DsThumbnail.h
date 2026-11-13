/*
 * DsThumbnail.h
 *
 * <ds_thumbnail>: an image as tall as its parent's content and as wide as its aspect ratio needs. Its attributes are in
 * registerXml().
 */

#pragma once

#include "UI/Lvgl/LvImage.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsThumbnail : public LvImage
	{
	  public:
		/** Create an empty thumbnail inside `parent` */
		DsThumbnail(const std::string& name, lv_obj_t* parent);

		/** Stop following the parent's size */
		~DsThumbnail() override;

		/** Register `<ds_thumbnail>` with the XML engine */
		static void registerXml();

		/** Show the image at `path`, or none if it is empty */
		void setImage(std::string_view path);

	  private:
		/** Pass a temporary C++ wrapper for `parent` to LvImage, which only uses it during construction */
		DsThumbnail(const std::string& name, LvObjRef&& parent);

		/** `<ds_thumbnail>` create callback: create the C++ thumbnail, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_thumbnail>` apply callback: lv_image attributes, then `bind_src` */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);

		/** `bind_src` observer */
		static void srcObserverCb(lv_observer_t* observer, lv_subject_t* subject);

		/** Something in the parent changed size or place: fit the parent's content height again */
		static void parentChangedCb(lv_event_t* e);

		/** Set the width for the parent's content height and the image's aspect ratio */
		void fit();

		lv_obj_t* m_parent; // The parent, whose changes are followed
		std::string m_path; // Shown image path; LVGL keeps a pointer to it
	};
} // namespace UI::Xml

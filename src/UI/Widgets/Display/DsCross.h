/*
 * DsCross.h
 *
 * <ds_cross>: an X drawn corner to corner across its content area, in its line style, to cross something out (such
 * as a cancelled object on the bed). Its attributes are in registerXml().
 */

#pragma once

#include "UI/Lvgl/LvObj.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml
{
	class DsCross : public LvObj
	{
	  public:
		/** Create a cross inside `parent` */
		DsCross(const std::string& name, lv_obj_t* parent);

		/** Register `<ds_cross>` with the XML engine */
		static void registerXml();

	  private:
		/** Draw the two lines, over the background */
		static void drawCb(lv_event_t* e);

		/** `<ds_cross>` create callback: create the C++ cross, deleted with its widget */
		static void* xmlCreate(lv_xml_parser_state_t* state, const char** attrs);

		/** `<ds_cross>` apply callback: lv_obj attributes */
		static void xmlApply(lv_xml_parser_state_t* state, const char** attrs);
	};
} // namespace UI::Xml

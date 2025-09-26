/*
 * LvXml.h
 *
 *  Created on: 2025-08-21
 *      Author: Andy Everitt
 */

#pragma once

#include "UI/Components/LVGL/LvContainer.h"

#if LV_USE_XML
namespace UI
{
	class LvXml : public LvObj
	{
	  public:
		LvXml(const std::string& name, const std::string& component_id, LvObj& parent, const char** attrs = nullptr);

		static void registerComponentFromFile(std::string_view path);

		void registerSubject(std::string_view subject_name, lv_subject_t* subject);
		lv_subject_t* getSubject(std::string_view subject_name) const;

	  private:
		std::string m_componentId;
        lv_xml_component_scope_t* m_scope;
	};
} // namespace UI

#endif
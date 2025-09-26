/*
 * LvXml.cpp
 *
 *  Created on: 2025-08-21
 *      Author: Andy Everitt
 */

#include "LvXml.h"
#include "Debug.h"

#if LV_USE_XML
namespace UI
{
	LvXml::LvXml(const std::string& name, const std::string& component_id, LvObj& parent, const char** attrs)
		: LvObj(
			  [component_id, attrs](LvObj& parent)
			  {
				  void* obj = lv_xml_create(parent, component_id.c_str(), attrs);
				  assert(obj != nullptr);
				  return static_cast<lv_obj_t*>(obj);
			  },
			  name,
			  parent)
		, m_componentId(component_id)
		, m_scope(lv_xml_component_get_scope(component_id.c_str()))
	{
	}

	void LvXml::registerComponentFromFile(std::string_view path)
	{
		UI_LOCK();
		lv_result_t result = lv_xml_component_register_from_file(path.data());
		assert(result == LV_RESULT_OK);
	}
} // namespace UI

#endif
/*
 * DsIcon.cpp
 */

#include "DsIcon.h"
#include "UI/Styles/Styles.h"
#include "WidgetSupport.h"
#include "Widgets.h"
#include "src/xml/parsers/lv_xml_image_parser.h"

namespace UI::Xml
{
	using namespace WidgetSupport;

	DsIcon::DsIcon(const std::string& name, lv_obj_t* parent)
		: DsIcon(name, LvObjRef(parent))
	{
	}

	DsIcon::DsIcon(const std::string& name, LvObjRef&& parent)
		: Icon(name, parent)
	{
	}

	void DsIcon::registerXml()
	{
		Widgets::add({.name = "ds_icon",
					  .extends = "lv_image",
					  .description = "An icon from the icon set chosen in the settings, coloured by the theme. It "
									 "follows icon set changes.",
					  .example = R"(<ds_icon icon="folder.png" width="72" height="72"/>)",
					  .attributes = {{"icon", "Icon file from the icon set, e.g. \"folder.png\""},
									 {"folder",
									  "Take `icon` from this folder of assets/icons instead, whatever the icon set, "
									  "e.g. \"hardware_test\""}}},
					 xmlCreate,
					 xmlApply);
	}

	void* DsIcon::xmlCreate(lv_xml_parser_state_t* state, const char** attrs)
	{
		auto* icon = new DsIcon(attrOr(attrs, "name", "icon"), parentOf(state));
		deleteWith(icon->getRootPtr(), icon);
		return icon->getRootPtr();
	}

	void DsIcon::xmlApply(lv_xml_parser_state_t* state, const char** attrs)
	{
		lv_xml_image_apply(state, attrs);
		if (const char* icon = lv_xml_get_value_of(attrs, "icon"))
		{
			auto& item = static_cast<DsIcon&>(*LvObj::fromPtr(itemOf(state)));
			if (const char* folder = lv_xml_get_value_of(attrs, "folder"))
			{
				item.setFixedIcon(Themes::getFixedIconPath(folder, icon));
			}
			else
			{
				item.setIcon(icon);
			}
		}
	}
} // namespace UI::Xml

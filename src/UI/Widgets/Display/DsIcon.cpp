/*
 * DsIcon.cpp
 */

#include "DsIcon.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/WidgetSupport.h"
#include "UI/Widgets/Widgets.h"
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
									 {"bind_icon",
									  "Follow a text subject naming the icon file, e.g. a WiFi network's signal",
									  AttributeKind::Subject},
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
		if (lv_subject_t* subject = findSubject(state, attrs, "bind_icon"))
		{
			lv_obj_t* root = itemOf(state);
			lv_subject_add_observer_obj(subject, iconObserverCb, root, LvObj::fromPtr(root));
		}
	}

	void DsIcon::iconObserverCb(lv_observer_t* observer, lv_subject_t* subject)
	{
		auto* icon = static_cast<DsIcon*>(lv_observer_get_user_data(observer));
		icon->setIcon(lv_subject_get_string(subject));
	}
} // namespace UI::Xml

/*
 * ThemeStyles.cpp
 */

#include "ThemeStyles.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "helix_xml.h"
#include <string>
#include <utility>

namespace UI::Xml::ThemeStyles
{
	/** The LVGL symbols themes use as images, by XML name */
	static constexpr std::pair<const char*, const char*> s_images[] = {
		{"symbol_ok", LV_SYMBOL_OK},
	};

	void registerAll()
	{
		ZoneScoped;
		Themes::forEachStyle(
			[](std::string_view name, Themes::Style& style)
			{
				const std::string terminated(name);
				lv_xml_register_style_external(nullptr, terminated.c_str(), static_cast<lv_style_t*>(style));
			});

		const Themes::Fonts& fonts = Themes::getFonts();
		for (const auto& role : Themes::FONT_ROLES)
		{
			lv_xml_register_font(nullptr, role.xmlName, &(fonts.*role.member));
		}

		for (const auto& [name, symbol] : s_images)
		{
			lv_xml_register_image(nullptr, name, symbol);
		}
	}

	std::vector<std::string_view> fontNames()
	{
		std::vector<std::string_view> names;
		for (const auto& role : Themes::FONT_ROLES)
		{
			names.emplace_back(role.xmlName);
		}
		return names;
	}

	std::vector<std::string_view> imageNames()
	{
		std::vector<std::string_view> names;
		for (const auto& [name, symbol] : s_images)
		{
			names.emplace_back(name);
		}
		return names;
	}
} // namespace UI::Xml::ThemeStyles

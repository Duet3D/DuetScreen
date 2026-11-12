/*
 * ThemeStyles.cpp
 */

#include "ThemeStyles.h"
#include "UI/Styles/Styles.h"
#include "helix_xml.h"
#include <string>

namespace UI::Xml::ThemeStyles
{
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
		lv_xml_register_font(nullptr, "font_header", &fonts.header);
		lv_xml_register_font(nullptr, "font_normal", &fonts.normal);
		lv_xml_register_font(nullptr, "font_emphasis", &fonts.emphasis);
		lv_xml_register_font(nullptr, "font_subdued", &fonts.subdued);
	}
} // namespace UI::Xml::ThemeStyles

/*
 * Translations.cpp
 */

#include "Translations.h"
#include "Debug.h"
#include "i18n/i18n.h"
#include "lvgl/lvgl.h"
#include <set>
#include <string>

namespace UI::Xml::Translations
{
	static std::set<std::string> s_languages; // Languages already given to LVGL

	/** Give LVGL every translation of `language`, as one pack */
	static void addLanguage(const std::string& language)
	{
		lv_translation_pack_t* pack = lv_translation_add_dynamic();
		lv_translation_add_language(pack, language.c_str());
		i18n::forEachTranslation(
			[pack](std::string_view key, std::string_view value)
			{
				const std::string k(key);
				const std::string v(value);
				if (lv_translation_tag_dsc_t* tag = lv_translation_add_tag(pack, k.c_str()))
				{
					lv_translation_set_tag_translation(pack, tag, 0, v.c_str());
				}
			});
		s_languages.insert(language);
	}

	void sync()
	{
		ZoneScoped;
		std::string language(i18n::getCurrentLanguage());
		if (language.empty())
		{
			language = "default";
		}
		if (!s_languages.contains(language))
		{
			addLanguage(language);
		}
		lv_translation_set_language(language.c_str());
	}

	void reset()
	{
		s_languages.clear();
	}
} // namespace UI::Xml::Translations

/*
 * Translations.h
 *
 * `translation_tag` attributes in XML are resolved by LVGL's translation module. This feeds it DuetScreen's
 * translation table (assets/i18n) for the current language, so XML and C++ views use the same text.
 */

#pragma once

namespace UI::Xml::Translations
{
	/** Give LVGL the current language's translations (built the first time a language is used) and select it */
	void sync();

	/** Forget the translations given to LVGL; lv_deinit() frees them */
	void reset();
} // namespace UI::Xml::Translations

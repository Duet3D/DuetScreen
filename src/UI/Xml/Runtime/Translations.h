/*
 * Translations.h
 *
 * Translation keys in XML and widget files are written `tr.<key>`, e.g. `translation_tag="tr.fan.max"`, so a reader
 * can tell a key from a subject name: the key is `fan.max` in assets/i18n/en-GB.json. `translation_tag` attributes are
 * resolved by LVGL's translation module, which this feeds DuetScreen's translation table for the current language,
 * every key registered as `tr.<key>`, so XML and C++ views use the same text. Everything else that takes a key from
 * XML or a widget file reads it with key() or translate().
 */

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace UI::Xml::Translations
{
	/** What a translation key is written after, in XML and widget files */
	inline constexpr std::string_view PREFIX = "tr.";

	/**
	 * Stop the XML engine implying a translation key from literal text (it would read `text="XY"` as
	 * `translation_tag="XY"`): literal text is shown as written, and translated text names its key, `tr.<key>`
	 */
	void init();

	/** Give LVGL the current language's translations (built the first time a language is used) and select it */
	void sync();

	/** Forget the translations given to LVGL; lv_deinit() frees them */
	void reset();

	/** The key `tag` (`tr.<key>`) names, or nothing if it isn't written `tr.<key>` */
	std::optional<std::string_view> key(std::string_view tag);

	/** The translation of `tag` (`tr.<key>`); empty for an empty tag, and with a warning if it isn't written `tr.<key>` */
	std::string translate(std::string_view tag);
} // namespace UI::Xml::Translations

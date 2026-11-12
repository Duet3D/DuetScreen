/*
 * LvI18n.h
 *
 *  Created on: 2025-10-13
 *      Author: Andy Everitt
 */

#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <functional>
#include <map>
#include <string_view>
#include <vector>

namespace i18n
{
	/* Load translation files from `assets/i18n` */
	void init();

	std::string_view getCurrentLanguage();
	void refreshLanguageFiles();
	const std::map<std::string, std::string>& getAvailableLanguages();
	int32_t getLanguageIndex(std::string_view lang);

	bool setLanguage(const std::string_view lang);

	const std::string& translate(std::string_view tag);

	/** Visit every key/value of the current language's translation table */
	void forEachTranslation(const std::function<void(std::string_view key, std::string_view value)>& func);

	/** Called after `setLanguage()` loads a new language. Only one listener is kept; pass `nullptr` to clear it. */
	void setLanguageChangedCallback(std::function<void()> callback);

	const std::vector<std::string>& getSupportedFonts();
} // namespace i18n

/**
 * @brief Get the translated string in the current language for a given key
 * @param tag The translation key, separated by `.` for nested keys
 * @return The translated string
 */
static inline const std::string& _(std::string_view tag)
{
	return i18n::translate(tag);
}

template <typename... Args>
	requires(sizeof...(Args) > 0)
std::string _(std::string_view tag, Args&&... args)
{
	/* Requires `#include "Debug.h"` before `#include "i18n/i18n.h"` */
	LOG_DBG("Formatting translation tag '{:s}' with {} arguments", tag, sizeof...(Args));
	return fmt::format(fmt::runtime(i18n::translate(tag)), std::forward<Args>(args)...);
}

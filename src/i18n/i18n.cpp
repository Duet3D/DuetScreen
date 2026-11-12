/*
 * i18n.cpp
 *
 *  Created on: 2025-10-13
 *      Author: Andy Everitt
 */

#include "Debug.h"

#include "Configuration.h"
#include "i18n.h"
#include "utils/StorageHelper.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace i18n
{
#if SIMULATION
#  define LANG_DIR "assets/i18n"
#else
#  define LANG_DIR "/etc/assets/i18n"
#endif
#define LANG_FILE_EXT ".json"
	using language_code_t = std::string;
	using language_readable_t = std::string;
	using language_table_t = std::map<language_code_t, language_readable_t>;
	static language_code_t s_currentLanguage;
	static language_table_t s_languages;
	static std::vector<std::string> s_supportedFonts;

	struct string_hash
	{
		using is_transparent = void; // Enable heterogeneous lookup

		size_t operator()(const std::string& str) const noexcept { return std::hash<std::string_view>{}(str); }
		size_t operator()(std::string_view str) const noexcept { return std::hash<std::string_view>{}(str); }
		size_t operator()(const char* str) const noexcept { return std::hash<std::string_view>{}(str); }
	};

	using translation_key_t = std::string;
	using translation_value_t = std::string;
	using translation_table_t =
		std::unordered_map<translation_key_t, translation_value_t, string_hash, std::equal_to<>>;
	static translation_table_t s_translationTable;
	static std::function<void()> s_languageChangedCallback;

	static bool loadLanguageFile(const std::filesystem::path& filepath);
	static nlohmann::json parseLanguageFile(const std::filesystem::path& filepath);
	static std::string getLanguageReadableName(const nlohmann::json& contents);
	static std::vector<std::string> getLanguageFonts(const nlohmann::json& contents);

	void init()
	{
		ZoneScoped;
		LOG_INFO("Initialising i18n module...");
		// Defer refreshLanguageFiles() until getAvailableLanguages() is first called.
		// At startup we only need the active language loaded.
		const auto lang_code = StorageHelper::getData(ID_SYS_LANG_CODE_KEY);
		if (!setLanguage(lang_code))
		{
			LOG_ERROR("Failed to set language to '{:s}', falling back to '{:s}'",
					  lang_code,
					  ID_SYS_LANG_CODE_KEY.default_value);
			if (!setLanguage(ID_SYS_LANG_CODE_KEY.default_value))
			{
				LOG_ERROR("Failed to load default language file");
			}
		}
	}

	std::string_view getCurrentLanguage()
	{
		ZoneScoped;
		return s_currentLanguage;
	}

	void refreshLanguageFiles()
	{
		ZoneScoped;
		LOG_DBG("Refreshing language files...");
		s_languages.clear();
		std::ranges::for_each(std::filesystem::directory_iterator(LANG_DIR),
							  [](const auto& entry)
							  {
								  auto& path = entry.path();
								  if (entry.is_regular_file() && path.extension() == LANG_FILE_EXT)
								  {
									  std::string lang_code = path.stem().string();
									  nlohmann::json contents = parseLanguageFile(path);
									  if (contents.is_null())
									  {
										  LOG_DBG("Skipping invalid language file: {:s}", path.c_str());
										  return;
									  }

									  std::string readable = getLanguageReadableName(contents);
									  if (readable.empty())
									  {
										  LOG_WARN("Language file {:s} missing readable name, skipping", path.c_str());
										  return;
									  }
									  s_languages[lang_code] = readable;
								  }
							  });
	}

	const std::map<language_code_t, language_readable_t>& getAvailableLanguages()
	{
		ZoneScoped;
		if (s_languages.empty())
		{
			refreshLanguageFiles();
		}
		return s_languages;
	}

	bool setLanguage(const std::string_view lang)
	{
		ZoneScoped;
		LOG_INFO("Setting language to {:s}", lang);
		std::filesystem::path lang_file = fmt::format(LANG_DIR "/{:s}" LANG_FILE_EXT, lang);
		if (!std::filesystem::exists(lang_file))
		{
			LOG_ERROR("Language file not found: {:s}", lang);
			return false;
		}

		if (!loadLanguageFile(lang_file))
		{
			LOG_ERROR("Failed to load language file: {:s}", lang_file.string());
			return false;
		}

		s_currentLanguage = lang;
		if (s_languageChangedCallback)
		{
			s_languageChangedCallback();
		}
		return true;
	}

	void forEachTranslation(const std::function<void(std::string_view key, std::string_view value)>& func)
	{
		ZoneScoped;
		for (const auto& [key, value] : s_translationTable)
		{
			func(key, value);
		}
	}

	void setLanguageChangedCallback(std::function<void()> callback)
	{
		s_languageChangedCallback = std::move(callback);
	}

	const std::string& translate(std::string_view tag)
	{
		ZoneScoped;
		/* Find the translation if it exists */
		{
			auto it = s_translationTable.find(tag);
			if (it != s_translationTable.end())
			{
				return it->second;
			}
		}

		/* Add the missing translation key to the table */
		{
			LOG_WARN("Missing translation for key '{:s}'", tag);
			auto [it, inserted] = s_translationTable.emplace(std::string(tag), std::string(tag));
			if (!inserted)
			{
				LOG_FATAL_THROW("Failed to insert missing translation key '{:s}'", tag);
			}
			return it->second;
		}
	}

	int32_t getLanguageIndex(std::string_view lang)
	{
		ZoneScoped;
		const auto& langs = getAvailableLanguages();
		int32_t index = 0;
		for (const auto& [code, readable] : langs)
		{
			if (code == lang)
			{
				return index;
			}
			++index;
		}
		return -1;
	}

	const std::vector<std::string>& getSupportedFonts()
	{
		ZoneScoped;
		return s_supportedFonts;
	}

	static bool loadLanguageFile(const std::filesystem::path& filepath)
	{
		ZoneScoped;
		nlohmann::json data = parseLanguageFile(filepath);
		if (data.is_null())
		{
			return false;
		}

		std::string readable = getLanguageReadableName(data);

		if (data.find("translations") == data.end() || !data["translations"].is_object())
		{
			LOG_ERROR("Language file missing 'translations' field or it is not an object: {:s}", filepath.string());
			return false;
		}
		auto translations = data["translations"].get<std::map<std::string, nlohmann::json>>();

		// Extract language code from filename
		std::string lang_code = filepath.filename().stem().string();

		// Extract supported fonts
		s_supportedFonts = getLanguageFonts(data);

		// Add to translation table
		std::string full_key;
		// Flatten arbitrarily nested translation objects into dot-separated keys
		s_translationTable.clear();

		auto flatten = [&](const auto& self, const nlohmann::json& node, const std::string& prefix) -> void
		{
			if (node.is_object())
			{
				for (const auto& [k, v] : node.items())
				{
					std::string next = prefix.empty() ? k : (prefix + "." + k);
					self(self, v, next);
				}
			}
			else if (node.is_string())
			{
				s_translationTable[prefix] = node.get<std::string>();
			}
			else
			{
				LOG_WARN("Translation value for key '{}' is not a string, skipping", prefix);
			}
		};

		flatten(flatten, data["translations"], "");

		return true;
	}

	static nlohmann::json parseLanguageFile(const std::filesystem::path& filepath)
	{
		ZoneScoped;
		LOG_DBG("Parsing language file: {:s}...", filepath.string());
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			LOG_ERROR("Failed to open language file: {:s}", filepath.string());
			return {};
		}

		nlohmann::json j;
		try
		{
			/* Parse json and ignore comments */
			j = nlohmann::json::parse(file,
									  /* callback */ nullptr,
									  /* allow_exceptions */ true,
									  /* ignore_comments */ true);
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Error reading JSON file: {}", e.what());
			return {};
		}

		return j;
	}

	static std::string getLanguageReadableName(const nlohmann::json& contents)
	{
		ZoneScoped;
		if (contents.find("readable") != contents.end() && contents["readable"].is_string())
		{
			return contents["readable"].get<std::string>();
		}
		return "";
	}

	static std::vector<std::string> getLanguageFonts(const nlohmann::json& contents)
	{
		ZoneScoped;
		if (contents.find("fonts") != contents.end() && contents["fonts"].is_array())
		{
			return contents["fonts"].get<std::vector<std::string>>();
		}
		return {};
	}
} // namespace i18n

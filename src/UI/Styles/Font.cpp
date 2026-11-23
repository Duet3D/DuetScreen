/*
 * Font.cpp
 *
 *  Created on: 2025-10-23
 *      Author: Andy Everitt
 */

#include "Font.h"
#include "Debug.h"
#include "Theme.h"
#include "UI/Styles/Styles.h"
#include "utils/StorageHelper.h"
#include <filesystem>

#define FONTS_FOLDER ASSETS_FOLDER "fonts/"
#define FONT_MANAGER_STATIC_BOLD 0

namespace UI::FontManager
{
	static lv_font_manager_t* s_fontManager = nullptr;
	static std::vector<std::string> s_loadedFontNames;
	static std::string s_activeTypeface;

	Font::Font(const lv_font_t* font)
		: m_font(font)
	{
		ZoneScoped;
	}

	Font::~Font()
	{
		ZoneScoped;
		if (m_font && m_font != LV_FONT_DEFAULT && s_fontManager)
		{
			lv_font_manager_delete_font(s_fontManager, const_cast<lv_font_t*>(m_font));
			m_font = nullptr;
		}
	}

	Font::Font(Font&& other) noexcept
		: m_font(other.m_font)
	{
		ZoneScoped;
		other.m_font = nullptr;
	}

	Font& Font::operator=(Font&& other) noexcept
	{
		ZoneScoped;
		if (this != &other)
		{
			if (m_font && m_font != LV_FONT_DEFAULT && s_fontManager)
			{
				lv_font_manager_delete_font(s_fontManager, const_cast<lv_font_t*>(m_font));
			}
			m_font = other.m_font;
			other.m_font = nullptr;
		}
		return *this;
	}

	/**
	 * @brief Load all fonts from the fonts directory
	 */
	void init()
	{
		ZoneScoped;
		LOG_INFO("Initializing Font Manager...");
		s_fontManager = lv_font_manager_create(8);

		std::filesystem::directory_iterator dir_iter(FONTS_FOLDER);
		for (const auto& entry : dir_iter)
		{
			if (!entry.is_regular_file())
			{
				continue;
			}
			const auto& path = entry.path();
			const std::string name = path.stem().string();

			bool success = false;
			if (path.extension() == ".bin")
			{
				LOG_DBG("Loading .bin font: {:s}", path.filename().c_str());
				success = lv_font_manager_add_src(s_fontManager, name.c_str(), path.c_str(), &lv_binfont_font_class);
			}
			else if (path.extension() == ".ttf")
			{
				LOG_DBG("Loading FreeType font: {:s}", path.filename().c_str());
				success = lv_font_manager_add_src(s_fontManager, name.c_str(), path.c_str(), &lv_freetype_font_class);
			}
			else
			{
				LOG_VERBOSE("Skipping unsupported font file: {:s}", path.filename().c_str());
				continue;
			}

			if (!success)
			{
				LOG_ERROR("Failed to load font: {:s}", path.c_str());
				continue;
			}

			LOG_VERBOSE("Loaded font: {:s}", name);
			s_loadedFontNames.push_back(name);
		}

		std::string selected_font(StorageHelper::getData(ID_FONT));
		if (isFontLoaded(selected_font))
		{
			s_activeTypeface = selected_font;
		}
	}

	Font createFont(const std::string& typeface, uint32_t size, uint32_t style)
	{
		ZoneScoped;
		if (!s_fontManager)
		{
			LOG_ERROR("Font manager not initialized");
			return nullptr;
		}

#if FONT_MANAGER_STATIC_BOLD
		// Choose the best available face variant based on style
		std::string chosenName = typeface;
		const bool wantBold = (style & LV_FREETYPE_FONT_STYLE_BOLD) != 0;
		if (wantBold)
		{
			std::string boldName = typeface + "-Bold";
			if (isFontLoaded(boldName))
			{
				chosenName = boldName;
			}
			else
			{
				std::string regularName = typeface + "-Regular";
				if (isFontLoaded(regularName))
				{
					chosenName = regularName;
				}
				else if (!isFontLoaded(typeface))
				{
					// Neither bold nor regular (nor base) loaded, fall back to default
					return LV_FONT_DEFAULT;
				}
			}
		}
		else
		{
			std::string regularName = typeface + "-Regular";
			if (isFontLoaded(regularName))
			{
				chosenName = regularName;
			}
			else if (!isFontLoaded(typeface))
			{
				// Neither regular nor base loaded, fall back to default
				return LV_FONT_DEFAULT;
			}
		}

		Font font(lv_font_manager_create_font(
			s_fontManager, chosenName.c_str(), LV_FREETYPE_FONT_RENDER_MODE_BITMAP, size, style, LV_FONT_KERNING_NONE));
		if (!font.get())
		{
			LOG_ERROR("Failed to create font '{:s}' with size {}", chosenName, size);
		}
#else
		lv_font_t* font = lv_font_manager_create_font(
			s_fontManager, typeface.c_str(), LV_FREETYPE_FONT_RENDER_MODE_BITMAP, size, style, LV_FONT_KERNING_NONE);
		if (!font)
		{
			LOG_ERROR("Failed to create font '{:s}' with size {}", typeface, size);
			return LV_FONT_DEFAULT;
		}
#endif
		return font;
	}

	const std::vector<std::string>& getLoadedFontNames()
	{
		ZoneScoped;
		return s_loadedFontNames;
	}

	bool isFontLoaded(const std::string& name)
	{
		ZoneScoped;
		return std::find(s_loadedFontNames.begin(), s_loadedFontNames.end(), name) != s_loadedFontNames.end();
	}

	const std::string& getActiveTypefaceName()
	{
		ZoneScoped;
		return s_activeTypeface;
	}

	void setActiveTypeface(const std::string& name)
	{
		ZoneScoped;
		if (name == s_activeTypeface)
		{
			return;
		}

		if (!isFontLoaded(name))
		{
			LOG_ERROR("Font '{:s}' not loaded, cannot set as active typeface", name);
			return;
		}
		LOG_DBG("Setting typeface to '{:s}'", name);

		s_activeTypeface = name;
		Themes::setTypeface(name);

		lv_obj_refresh_style(lv_screen_active(), LV_PART_ANY, LV_STYLE_PROP_ANY);

		StorageHelper::setData(ID_FONT, s_activeTypeface);
		LOG_INFO("Active typeface set to '{:s}'", name);
	}
} // namespace UI::FontManager

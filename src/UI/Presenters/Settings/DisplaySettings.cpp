/*
 * DisplaySettings.cpp
 */

#include "DisplaySettings.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Font.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "i18n/i18n.h"
#include "utils/DisplayHelper.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <fmt/format.h>
#include <string>
#include <vector>

namespace UI::DisplaySettings
{
	namespace
	{
		/** A screen rotation offered */
		struct Rotation
		{
			std::string_view name;	  // Translation key
			DisplayRotation rotation; // The rotation
		};

		/** The rotations offered: 90° and 270° don't fit the layouts */
		constexpr std::array ROTATIONS = {
			Rotation{"settings.screen_rotation_options.rotation_0", DisplayRotation::ROTATION_0},
			Rotation{"settings.screen_rotation_options.rotation_180", DisplayRotation::ROTATION_180},
		};

		/** The index of `value` in `items`, or 0 if it isn't there */
		int32_t indexOf(const std::vector<std::string>& items, std::string_view value)
		{
			const auto it = std::find(items.begin(), items.end(), value);
			return it == items.end() ? 0 : static_cast<int32_t>(it - items.begin());
		}

		/** The themes, by name, in the order offered */
		std::vector<std::string> themeNames()
		{
			std::vector<std::string> names;
			for (const auto& [name, theme] : Themes::getThemes())
			{
				names.emplace_back(name);
			}
			return names;
		}

		/** `items` translated under `prefix`, one per line */
		std::string translated(const std::vector<std::string>& items, std::string_view prefix)
		{
			std::vector<std::string> names;
			for (const auto& item : items)
			{
				names.push_back(_(fmt::format("{:s}{:s}", prefix, item)));
			}
			return Xml::Settings::lines(names);
		}
	} // namespace

	void registerAll()
	{
		Xml::Settings::add({.name = "theme",
							.description = "Index of the theme in use",
							.read = [] { return indexOf(themeNames(), StorageHelper::getData(ID_THEME)); },
							.write =
								[](int32_t index)
							{
								auto* theme = Themes::getTheme(static_cast<size_t>(index));
								if (theme == nullptr)
								{
									return;
								}
								theme->setThemeActive();
								StorageHelper::setData(ID_THEME, theme->getName());
								Model::get().post<EventType::ThemeChanged>();
							},
							.options = [] { return translated(themeNames(), "theme.id."); }});
		Xml::Settings::add(
			{.name = "font",
			 .description = "Index of the typeface in use",
			 .read = [] { return indexOf(FontManager::getLoadedFontNames(), FontManager::getActiveTypefaceName()); },
			 .write =
				 [](int32_t index)
			 {
				 const auto fonts = FontManager::getLoadedFontNames();
				 if (index >= 0 && static_cast<size_t>(index) < fonts.size())
				 {
					 FontManager::setActiveTypeface(fonts[static_cast<size_t>(index)]);
				 }
			 },
			 .options = [] { return Xml::Settings::lines(FontManager::getLoadedFontNames()); }});
		Xml::Settings::add({.name = "icon_set",
							.description = "Index of the icon set in use",
							.read = [] { return indexOf(Themes::getIconSets(), Themes::getIconFolder()); },
							.write =
								[](int32_t index)
							{
								const auto sets = Themes::getIconSets();
								if (index >= 0 && static_cast<size_t>(index) < sets.size())
								{
									Themes::setIconFolder(sets[static_cast<size_t>(index)]);
								}
							},
							.options = [] { return translated(Themes::getIconSets(), "theme.icon_sets."); }});
		Xml::Settings::add({.name = "rotation",
							.description = "Index of the screen's rotation",
							.read =
								[]
							{
								for (size_t i = 0; i < ROTATIONS.size(); i++)
								{
									if (ROTATIONS[i].rotation == DisplayHelper::getRotation())
									{
										return static_cast<int32_t>(i);
									}
								}
								return 0;
							},
							.write =
								[](int32_t index)
							{
								if (index >= 0 && static_cast<size_t>(index) < ROTATIONS.size())
								{
									DisplayHelper::setRotation(ROTATIONS[static_cast<size_t>(index)].rotation);
								}
							},
							.options =
								[]
							{
								std::vector<std::string> names;
								for (const auto& rotation : ROTATIONS)
								{
									names.push_back(_(rotation.name));
								}
								return Xml::Settings::lines(names);
							}});
		Xml::Settings::add({.name = "animations",
							.description = "1 if the UI animates",
							.read = [] { return StorageHelper::getData(ID_UI_ANIMATIONS_ENABLED) ? 1 : 0; },
							.write = [](int32_t on) { StorageHelper::setData(ID_UI_ANIMATIONS_ENABLED, on != 0); }});
		Xml::Settings::add(
			{.name = "dark",
			 .description = "1 if the theme in use is in dark mode; it changes only if its palette can "
							"(source.theme.customisable)",
			 .read =
				 []
			 {
				 const Themes::Theme* theme = Themes::getCurrentTheme();
				 return theme != nullptr && theme->getPalette().dark ? 1 : 0;
			 },
			 .write =
				 [](int32_t on)
			 {
				 if (Themes::changeCurrentPalette([on](Themes::PaletteInputs& palette) { palette.dark = on != 0; }))
				 {
					 Model::get().post<EventType::ThemeChanged>();
				 }
			 }});
		Xml::Settings::add(
			{.name = "brightness",
			 .description = "The screen's brightness, 0-100",
			 .read = [] { return static_cast<int32_t>(DisplayHelper::getBrightness()); },
			 .write = [](int32_t value) { DisplayHelper::setBrightness(static_cast<unsigned int>(value)); }});
		Xml::Settings::add(
			{.name = "screensaver_timeout",
			 .description = "Seconds without a touch before the screen dims; 0 for never",
			 .read = [] { return static_cast<int32_t>(StorageHelper::getData(ID_SCREENSAVER_TIMEOUT).count()); },
			 .write = [](int32_t seconds)
			 { StorageHelper::setData(ID_SCREENSAVER_TIMEOUT, std::chrono::seconds(seconds)); }});
	}
} // namespace UI::DisplaySettings

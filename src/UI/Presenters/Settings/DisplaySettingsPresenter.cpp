/*
 * DisplaySettingsPresenter.cpp
 */

#include "DisplaySettingsPresenter.h"
#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "UI/Styles/Font.h"
#include "UI/Styles/Styles.h"
#include "UI/Styles/Theme.h"
#include "i18n/i18n.h"
#include "utils/DisplayHelper.h"
#include "utils/StorageHelper.h"
#include <array>
#include <fmt/format.h>

namespace UI
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

		/** Append `line` to the lines in `lines` */
		void addLine(std::string& lines, std::string_view line)
		{
			lines += lines.empty() ? "" : "\n";
			lines += line;
		}
	} // namespace

	DisplaySettingsPresenter::DisplaySettingsPresenter()
		: SettingsPresenter("settings_display")
	{
		std::string themes;
		for (const auto& [name, theme] : Themes::getThemes())
		{
			addLine(themes, _(fmt::format("theme.id.{:s}", name)));
		}
		m_themeNames.set(themes);

		m_fonts = FontManager::getLoadedFontNames();
		std::string fonts;
		for (const auto& font : m_fonts)
		{
			addLine(fonts, font);
		}
		m_fontNames.set(fonts);

		m_iconSets = Themes::getIconSets();
		std::string iconSets;
		for (const auto& iconSet : m_iconSets)
		{
			addLine(iconSets, _(fmt::format("theme.icon_sets.{:s}", iconSet)));
		}
		m_iconSetNames.set(iconSets);

		std::string rotations;
		for (const auto& rotation : ROTATIONS)
		{
			addLine(rotations, _(rotation.name));
		}
		m_rotations.set(rotations);

		action("settings.theme",
			   "Use the theme at the edited index of `themes`",
			   [this](const Xml::ActionArgs& args)
			   {
				   auto* theme = Themes::getTheme(static_cast<size_t>(args.value().value_or(-1)));
				   if (theme == nullptr)
				   {
					   return;
				   }
				   LOG_DBG("Changing theme");
				   m_theme.set(static_cast<int32_t>(args.value().value_or(0)));
				   theme->setThemeActive();
				   StorageHelper::setData(ID_THEME, theme->getName());
				   showDark();
				   Model::get().post<EventType::ThemeChanged>();
			   });
		action("settings.font",
			   "Use the typeface at the edited index of `fonts`",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto index = static_cast<size_t>(args.value().value_or(-1));
				   if (index < m_fonts.size())
				   {
					   m_font.set(static_cast<int32_t>(index));
					   FontManager::setActiveTypeface(m_fonts[index]);
				   }
			   });
		action("settings.icon_set",
			   "Use the icon set at the edited index of `icon_sets`",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto index = static_cast<size_t>(args.value().value_or(-1));
				   if (index < m_iconSets.size())
				   {
					   LOG_INFO("Changing icon set");
					   m_iconSet.set(static_cast<int32_t>(index));
					   Themes::setIconFolder(m_iconSets[index]);
				   }
			   });
		toggle("settings.animations",
			   "Turn UI animations on (edited value 1) or off (0)",
			   m_animations,
			   [](bool on) { StorageHelper::setData(ID_UI_ANIMATIONS_ENABLED, on); });
		toggle("settings.dark",
			   "Put the theme in use in dark mode (edited value 1) or light mode (0), if its palette can be changed",
			   m_dark,
			   [this](bool on)
			   {
				   if (Themes::changeCurrentPalette([on](Themes::PaletteInputs& palette) { palette.dark = on; }))
				   {
					   Model::get().post<EventType::ThemeChanged>();
				   }
				   showDark();
			   });
		setting("settings.brightness",
				"Set the screen brightness to the edited value, 0-100",
				[this](float value)
				{
					m_brightness.set(static_cast<int32_t>(value));
					DisplayHelper::setBrightness(static_cast<unsigned int>(value));
				});
		setting("settings.screensaver_timeout",
				"Dim the screen after the edited value, in seconds, without a touch; 0 for never",
				[this](float value)
				{
					m_screensaverTimeout.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_SCREENSAVER_TIMEOUT, std::chrono::seconds(static_cast<int32_t>(value)));
				});
		action("settings.rotation",
			   "Rotate the screen as the edited index of `rotations` says",
			   [this](const Xml::ActionArgs& args)
			   {
				   const auto index = static_cast<size_t>(args.value().value_or(-1));
				   if (index >= ROTATIONS.size())
				   {
					   LOG_ERROR("Invalid screen rotation index: {:d}", index);
					   return;
				   }
				   m_rotation.set(static_cast<int32_t>(index));
				   DisplayHelper::setRotation(ROTATIONS[index].rotation);
			   });
	}

	void DisplaySettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("themes", m_themeNames, "Themes offered, one per line");
		f.field("theme", m_theme, "Index of the theme in use");
		f.field("fonts", m_fontNames, "Typefaces offered, one per line");
		f.field("font", m_font, "Index of the typeface in use");
		f.field("icon_sets", m_iconSetNames, "Icon sets offered, one per line");
		f.field("icon_set", m_iconSet, "Index of the icon set in use");
		f.field("rotations", m_rotations, "Screen rotations offered, one per line");
		f.field("rotation", m_rotation, "Index of the rotation in use");
		f.field("animations", m_animations, "1 if the UI animates");
		f.field("brightness", m_brightness, "Brightness, 0-100");
		f.field(
			"screensaver_timeout", m_screensaverTimeout, "Seconds without a touch before the screen dims; 0 for never");
		f.field("dark", m_dark, "1 if the theme in use is in dark mode");
		f.field("dark_shown", m_darkShown, "1 if the theme in use can change between dark and light mode");
	}

	void DisplaySettingsPresenter::onActivate()
	{
		const std::string_view stored = StorageHelper::getData(ID_THEME);
		int32_t index = 0;
		for (const auto& [name, theme] : Themes::getThemes())
		{
			if (name == stored)
			{
				m_theme.set(index);
				break;
			}
			index++;
		}
		for (size_t i = 0; i < m_fonts.size(); i++)
		{
			if (m_fonts[i] == FontManager::getActiveTypefaceName())
			{
				m_font.set(static_cast<int32_t>(i));
			}
		}
		for (size_t i = 0; i < m_iconSets.size(); i++)
		{
			if (m_iconSets[i] == Themes::getIconFolder())
			{
				m_iconSet.set(static_cast<int32_t>(i));
			}
		}
		m_animations.set(StorageHelper::getData(ID_UI_ANIMATIONS_ENABLED) ? 1 : 0);
		m_brightness.set(static_cast<int32_t>(DisplayHelper::getBrightness()));
		m_screensaverTimeout.set(static_cast<int32_t>(StorageHelper::getData(ID_SCREENSAVER_TIMEOUT).count()));
		showDark();
		for (size_t i = 0; i < ROTATIONS.size(); i++)
		{
			if (ROTATIONS[i].rotation == DisplayHelper::getRotation())
			{
				m_rotation.set(static_cast<int32_t>(i));
			}
		}
	}

	void DisplaySettingsPresenter::showDark()
	{
		const Themes::Theme* theme = Themes::getCurrentTheme();
		const bool changeable = theme != nullptr && theme->isCustomisable();
		m_darkShown.set(changeable ? 1 : 0);
		m_dark.set(theme != nullptr && theme->getPalette().dark ? 1 : 0);
	}
} // namespace UI

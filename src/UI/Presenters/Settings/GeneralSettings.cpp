/*
 * GeneralSettings.cpp
 */

#include "GeneralSettings.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "ObjectModel/JobProgressSource.h"
#include "Storage.h"
#include "UI/Xml/Data/DataSources.h"
#include "UI/Xml/Data/SettingsSource.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include "utils/UnitSystem.h"
#include <array>
#include <iterator>
#include <string>
#include <vector>

namespace UI::GeneralSettings
{
	namespace
	{
		/** A USB keyboard layout: XKB layout code and variant */
		struct KeyboardLayout
		{
			std::string_view name;		   // Shown, in the layout's own language, so not translated
			std::string_view code;		   // XKB layout
			const char* variant = nullptr; // XKB variant, or none
		};

		/** The keyboard layouts offered */
		constexpr std::array KEYBOARD_LAYOUTS = {
			KeyboardLayout{.name = "US (QWERTY)", .code = "us"},
			KeyboardLayout{.name = "UK (QWERTY)", .code = "gb"},
			KeyboardLayout{.name = "Deutsch (QWERTZ)", .code = "de"},
			KeyboardLayout{.name = "Français (AZERTY)", .code = "fr"},
			KeyboardLayout{.name = "Español", .code = "es"},
			KeyboardLayout{.name = "Italiano", .code = "it"},
			KeyboardLayout{.name = "Svenska", .code = "se"},
			KeyboardLayout{.name = "Norsk", .code = "no"},
			KeyboardLayout{.name = "Dansk", .code = "dk"},
			KeyboardLayout{.name = "Nederlands", .code = "nl"},
			KeyboardLayout{.name = "Português", .code = "pt"},
			KeyboardLayout{.name = "Polski", .code = "pl"},
			KeyboardLayout{.name = "Čeština", .code = "cz"},
			KeyboardLayout{.name = "US Dvorak", .code = "us", .variant = "dvorak"},
		};

		/** How a job's progress can be worked out, in OM::JobProgressSource order */
		constexpr std::array<std::string_view, 2> PROGRESS_SOURCES = {
			"settings.job_progress_source_options.duration",
			"settings.job_progress_source_options.file",
		};

		/** Use `layout` for every USB keyboard */
		void applyKeyboardLayout(const KeyboardLayout& layout)
		{
#if LV_USE_EVDEV && LV_EVDEV_XKB
			struct xkb_rule_names names = {.rules = nullptr,
										   .model = "pc105",
										   .layout = layout.code.data(),
										   .variant = layout.variant,
										   .options = nullptr};
			lv_indev_t* indev = nullptr;
			while ((indev = lv_indev_get_next(indev)) != nullptr)
			{
				if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD)
				{
					lv_evdev_set_keymap(indev, names);
				}
			}
#else
			UNUSED(layout);
#endif
		}

		/** The index of the language in use */
		int32_t currentLanguage()
		{
			return i18n::getLanguageIndex(i18n::getCurrentLanguage());
		}

		/** Ask whether to restart in the language at `index`; no answer, or no, keeps the language in use */
		void askLanguage(int32_t index)
		{
			const auto& languages = i18n::getAvailableLanguages();
			if (index < 0 || static_cast<size_t>(index) >= languages.size() || index == currentLanguage())
			{
				return;
			}
			const std::string code = std::next(languages.begin(), index)->first;
			LOG_INFO("Language selection changed to: {:s}", code);
			Xml::Prompts::askQuestion({.title = _("settings.language_confirm.title"),
									   .text = _("settings.language_confirm.text"),
									   .onYes = [code]
									   {
										   StorageHelper::setData(ID_SYS_LANG_CODE_KEY, std::string_view(code));
										   Restart();
									   }});
		}
	} // namespace

	void registerAll()
	{
		Xml::Settings::add({.name = "language",
							.description = "Index of the language in use; choosing another asks to restart in it",
							.read = currentLanguage,
							.write = askLanguage,
							.options = []
							{
								std::vector<std::string> names;
								for (const auto& [code, readable] : i18n::getAvailableLanguages())
								{
									names.emplace_back(readable);
								}
								return Xml::Settings::lines(names);
							}});
		Xml::Settings::add({.name = "keyboard_layout",
							.description = "Index of the USB keyboard layout in use",
							.read =
								[]
							{
								const std::string_view code = StorageHelper::getData(ID_KEYBOARD_LAYOUT);
								for (size_t i = 0; i < KEYBOARD_LAYOUTS.size(); i++)
								{
									if (KEYBOARD_LAYOUTS[i].code == code)
									{
										return static_cast<int32_t>(i);
									}
								}
								return 0;
							},
							.write =
								[](int32_t index)
							{
								if (index < 0 || static_cast<size_t>(index) >= KEYBOARD_LAYOUTS.size())
								{
									return;
								}
								const KeyboardLayout& layout = KEYBOARD_LAYOUTS[static_cast<size_t>(index)];
								StorageHelper::setData(ID_KEYBOARD_LAYOUT, layout.code);
								applyKeyboardLayout(layout);
							},
							.options =
								[]
							{
								std::vector<std::string> names;
								for (const auto& layout : KEYBOARD_LAYOUTS)
								{
									names.emplace_back(layout.name);
								}
								return Xml::Settings::lines(names);
							}});
		Xml::Settings::add(
			{.name = "confirmations",
			 .description = "1 if actions on the machine are confirmed first",
			 .read = [] { return StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS) ? 1 : 0; },
			 .write = [](int32_t on) { StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, on != 0); }});
		Xml::Settings::add({.name = "machine_position_mode",
							.description = "1 if absolute moves use machine coordinates (G53)",
							.read = [] { return StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE) ? 1 : 0; },
							.write =
								[](int32_t on)
							{
								StorageHelper::setData(ID_MOVE_MACHINE_POSITION_MODE, on != 0);
								Xml::DataSources::updateAll(); // The positions shown change
							}});
		Xml::Settings::add({.name = "units",
							.description =
								"The unit the screen shows distances and speeds in: 0 millimetres, 1 inches. "
								"The Duet's own (G20/G21) is unaffected: moves are converted to it",
							.read = [] { return static_cast<int32_t>(StorageHelper::getData(ID_UNIT_SYSTEM)); },
							.write =
								[](int32_t index)
							{
								if (index == static_cast<int32_t>(Units::UnitSystem::Metric) ||
									index == static_cast<int32_t>(Units::UnitSystem::Imperial))
								{
									StorageHelper::setData(ID_UNIT_SYSTEM, Units::UnitSystem(index));
									// Every position, distance and speed shown changes unit
									Xml::DataSources::updateAll();
								}
							}});
		Xml::Settings::add({.name = "progress_source",
							.description = "Index of how a job's progress is worked out",
							.read = [] { return static_cast<int32_t>(StorageHelper::getData(ID_JOB_PROGRESS_SOURCE)); },
							.write =
								[](int32_t index)
							{
								if (index >= 0 && static_cast<size_t>(index) < PROGRESS_SOURCES.size())
								{
									StorageHelper::setData(ID_JOB_PROGRESS_SOURCE, OM::JobProgressSource(index));
								}
							},
							.options =
								[]
							{
								std::vector<std::string> names;
								for (const auto key : PROGRESS_SOURCES)
								{
									names.push_back(_(key));
								}
								return Xml::Settings::lines(names);
							}});
	}
} // namespace UI::GeneralSettings

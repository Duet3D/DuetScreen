/*
 * GeneralSettingsPresenter.cpp
 */

#include "GeneralSettingsPresenter.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "ObjectModel/JobProgressSource.h"
#include "Storage.h"
#include "UI/Xml/Dialogs/Prompts.h"
#include "i18n/i18n.h"
#include "utils/StorageHelper.h"
#include <array>
#include <fmt/format.h>

namespace UI
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
		constexpr std::string_view PROGRESS_SOURCES[] = {
			"settings.job_progress_source_options.duration",
			"settings.job_progress_source_options.file",
		};

		/** `items`, translated, one per line */
		template <typename R>
		std::string translatedLines(const R& items)
		{
			std::string lines;
			for (const auto& item : items)
			{
				lines += lines.empty() ? "" : "\n";
				lines += _(item);
			}
			return lines;
		}

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
	} // namespace

	GeneralSettingsPresenter::GeneralSettingsPresenter()
		: SettingsPresenter("settings_general")
	{
		std::string languages;
		for (const auto& [code, readable] : i18n::getAvailableLanguages())
		{
			languages += languages.empty() ? "" : "\n";
			languages += readable;
			m_languageCodes.push_back(code);
		}
		m_languages.set(languages);

		std::string layouts;
		for (const auto& layout : KEYBOARD_LAYOUTS)
		{
			layouts += layouts.empty() ? "" : "\n";
			layouts += layout.name;
		}
		m_keyboardLayouts.set(layouts);
		m_progressSources.set(translatedLines(PROGRESS_SOURCES));

		action("settings.language",
			   "Ask whether to restart in the language at the edited index of `languages`",
			   [this](const Xml::ActionArgs& args)
			   {
				   if (const auto value = args.value())
				   {
					   askLanguage(static_cast<size_t>(*value), args.target);
				   }
			   });
		setting("settings.keyboard_layout",
				"Use the USB keyboard layout at the edited index of `keyboard_layouts`",
				[this](float value)
				{
					const auto index = static_cast<size_t>(value);
					if (index >= KEYBOARD_LAYOUTS.size())
					{
						return;
					}
					m_keyboardLayout.set(static_cast<int32_t>(index));
					const KeyboardLayout& layout = KEYBOARD_LAYOUTS.at(index);
					LOG_INFO("Selected keyboard layout: {} (code: {}, variant: {})",
							 layout.name,
							 layout.code,
							 layout.variant ? layout.variant : "none");
					StorageHelper::setData(ID_KEYBOARD_LAYOUT, layout.code);
					applyKeyboardLayout(layout);
				});
		toggle("settings.confirmations",
			   "Ask first before acting on the machine (edited value 1), or not (0)",
			   m_confirmations,
			   [](bool on) { StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, on); });
		toggle("settings.machine_position_mode",
			   "Use machine coordinates (G53) for absolute moves if the edited value is 1, else not",
			   m_machinePositionMode,
			   [](bool on) { StorageHelper::setData(ID_MOVE_MACHINE_POSITION_MODE, on); });
		setting("settings.progress_source",
				"Work out job progress with the source at the edited index of `progress_sources`",
				[this](float value)
				{
					m_progressSource.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_JOB_PROGRESS_SOURCE, OM::JobProgressSource(static_cast<int>(value)));
				});
	}

	void GeneralSettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("languages", m_languages, "Languages offered, one per line");
		f.field("language", m_language, "Index of the language in use, in `languages`");
		f.field("keyboard_layouts", m_keyboardLayouts, "USB keyboard layouts, one per line");
		f.field("keyboard_layout", m_keyboardLayout, "Index of the layout in use, in `keyboard_layouts`");
		f.field("confirmations", m_confirmations, "1 if actions are confirmed first");
		f.field("machine_position_mode", m_machinePositionMode, "1 if absolute moves use machine coordinates (G53)");
		f.field("progress_sources", m_progressSources, "How a job's progress can be worked out, one per line");
		f.field("progress_source", m_progressSource, "Index of the one used, in `progress_sources`");
	}

	void GeneralSettingsPresenter::onActivate()
	{
		m_language.set(i18n::getLanguageIndex(i18n::getCurrentLanguage()));
		const std::string_view layout = StorageHelper::getData(ID_KEYBOARD_LAYOUT);
		int32_t layoutIndex = 0;
		for (size_t i = 0; i < KEYBOARD_LAYOUTS.size(); i++)
		{
			if (KEYBOARD_LAYOUTS.at(i).code == layout)
			{
				layoutIndex = static_cast<int32_t>(i);
				break;
			}
		}
		m_keyboardLayout.set(layoutIndex);
		m_confirmations.set(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS) ? 1 : 0);
		m_machinePositionMode.set(StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE) ? 1 : 0);
		m_progressSource.set(static_cast<int32_t>(StorageHelper::getData(ID_JOB_PROGRESS_SOURCE)));
	}

	void GeneralSettingsPresenter::askLanguage(size_t index, lv_obj_t* asker)
	{
		if (index >= m_languageCodes.size() || m_languageCodes.at(index) == i18n::getCurrentLanguage())
		{
			return;
		}
		LOG_INFO("Language selection changed to: {:s}", m_languageCodes.at(index));
		m_language.set(static_cast<int32_t>(index));
		Xml::Prompts::askQuestion(
			{.title = _("settings.language_confirm.title"),
			 .text = _("settings.language_confirm.text"),
			 .onYes =
				 [code = m_languageCodes.at(index)]
			 {
				 StorageHelper::setData(ID_SYS_LANG_CODE_KEY, std::string_view(code));
				 Restart();
			 },
			 // Not restarting keeps the language in use; the question goes with the list, and
			 // so with this
			 .onNo = [this] { m_language.set(i18n::getLanguageIndex(i18n::getCurrentLanguage())); },
			 .asker = asker});
	}
} // namespace UI

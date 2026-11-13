/*
 * GeneralSettingsPresenter.cpp
 */

#include "GeneralSettingsPresenter.h"
#include "BuildDate.h"
#include "Debug.h"
#include "Hardware/Reset.h"
#include "ObjectModel/Job.h"
#include "Storage.h"
#include "Subscribers/ResponseSubscribers.h"
#include "i18n/i18n.h"
#include "utils/DisplayHelper.h"
#include "utils/StorageHelper.h"
#include "utils/UpgradeHelper.h"
#include "version.h"
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
		: XmlPresenter("settings_general")
	{
#if USE_FIXED_TEST_STRINGS
		m_firmware.set(_("settings.build_time", "vX.Y.Z", "YYYY-MM-DD", "HH:mm::ss"));
#else
		m_firmware.set(_("settings.build_time", FIRMWARE_VERSION, BuildDateText, BuildTimeSuffix));
#endif
		m_buildroot.set(UpgradeHelper::getBuildrootVersion());

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
		m_notificationLevels.set(translatedLines(RESPONSE_TYPE_STRINGS));

		setting("settings.language",
				"Ask whether to restart in the language at the edited index of `languages`",
				[this](float value) { askLanguage(static_cast<size_t>(value)); });
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
		setting("settings.brightness",
				"Set the screen brightness to the edited value, 0-100",
				[this](float value)
				{
					m_brightness.set(static_cast<int32_t>(value));
					DisplayHelper::setBrightness(static_cast<unsigned int>(value));
				});
		setting("settings.screensaver_timeout",
				"Set the screensaver timeout to the edited value, in seconds",
				[this](float value)
				{
					m_screensaverTimeout.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_SCREENSAVER_TIMEOUT, std::chrono::seconds(static_cast<int32_t>(value)));
				});
		setting("settings.confirmations",
				"Turn confirmation dialogs on (edited value 1) or off (0)",
				[this](float value)
				{
					m_confirmations.set(value != 0.0f ? 1 : 0);
					StorageHelper::setData(ID_SHOW_CONFIRMATION_DIALOGS, value != 0.0f);
				});
		setting("settings.machine_position_mode",
				"Use machine coordinates (G53) for absolute moves if the edited value is 1, else not",
				[this](float value)
				{
					m_machinePositionMode.set(value != 0.0f ? 1 : 0);
					StorageHelper::setData(ID_MOVE_MACHINE_POSITION_MODE, value != 0.0f);
				});
		setting("settings.progress_source",
				"Work out job progress with the source at the edited index of `progress_sources`",
				[this](float value)
				{
					m_progressSource.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_JOB_PROGRESS_SOURCE, OM::JobProgressSource(static_cast<int>(value)));
				});
		setting("settings.connected_message",
				"Show a message on connecting if the edited value is 1, else not",
				[this](float value)
				{
					m_connectedMessage.set(value != 0.0f ? 1 : 0);
					StorageHelper::setData(ID_DISPLAY_CONNECTED_MESSAGE, value != 0.0f);
				});
		setting("settings.notification_level",
				"Show notifications from the level at the edited index of `notification_levels` up",
				[this](float value)
				{
					m_notificationLevel.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_NOTIFICATION_LEVEL, ResponseType(static_cast<int>(value)));
				});
		setting("settings.notification_timeout",
				"Close notifications after the edited value, in ms",
				[this](float value)
				{
					m_notificationTimeout.set(static_cast<int32_t>(value));
					StorageHelper::setData(ID_NOTIFICATION_TIMEOUT,
										   std::chrono::milliseconds(static_cast<int32_t>(value)));
				});
		setting("settings.error_notifications_stay",
				"Keep error notifications open until closed if the edited value is 1, else not",
				[this](float value)
				{
					m_errorNotificationsStay.set(value != 0.0f ? 1 : 0);
					StorageHelper::setData(ID_NOTIFICATION_AUTO_CLOSE_ERROR, value == 0.0f);
				});

		action("settings.restart_in_language",
			   "Answer yes to `language_confirm`: store the language picked and restart",
			   [this](const Xml::ActionArgs&)
			   {
				   m_languageConfirm.set(0);
				   StorageHelper::setData(ID_SYS_LANG_CODE_KEY, std::string_view(m_pickedLanguage));
				   Restart();
			   });
		action("settings.keep_language",
			   "Answer no to `language_confirm`: keep the language in use",
			   [this](const Xml::ActionArgs&)
			   {
				   m_languageConfirm.set(0);
				   m_language.set(i18n::getLanguageIndex(i18n::getCurrentLanguage()));
			   });
	}

	void GeneralSettingsPresenter::fields(Xml::FieldVisitor& f)
	{
		f.field("firmware", m_firmware, "Firmware version and build time");
		f.field("buildroot", m_buildroot, "Buildroot version");
		f.field("languages", m_languages, "Languages offered, one per line");
		f.field("language", m_language, "Index of the language in use, in `languages`");
		f.field("keyboard_layouts", m_keyboardLayouts, "USB keyboard layouts, one per line");
		f.field("keyboard_layout", m_keyboardLayout, "Index of the layout in use, in `keyboard_layouts`");
		f.field("brightness", m_brightness, "Brightness, 0-100");
		f.field("screensaver_timeout", m_screensaverTimeout, "Screensaver timeout, s");
		f.field("confirmations", m_confirmations, "1 if actions are confirmed first");
		f.field("machine_position_mode", m_machinePositionMode, "1 if absolute moves use machine coordinates (G53)");
		f.field("progress_sources", m_progressSources, "How a job's progress can be worked out, one per line");
		f.field("progress_source", m_progressSource, "Index of the one used, in `progress_sources`");
		f.field("connected_message", m_connectedMessage, "1 if a message is shown on connecting");
		f.field("notification_levels", m_notificationLevels, "Notification levels, one per line");
		f.field("notification_level", m_notificationLevel, "Index of the lowest level shown, in `notification_levels`");
		f.field("notification_timeout", m_notificationTimeout, "ms before a notification closes itself");
		f.field("error_notifications_stay", m_errorNotificationsStay, "1 if error notifications stay until closed");
		f.field("language_confirm", m_languageConfirm, "1 while asking to restart in another language");
		f.field("language_confirm_title", m_languageConfirmTitle, "The language question's title");
		f.field("language_confirm_text", m_languageConfirmText, "The language question");
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
		m_brightness.set(static_cast<int32_t>(DisplayHelper::getBrightness()));
		m_screensaverTimeout.set(static_cast<int32_t>(StorageHelper::getData(ID_SCREENSAVER_TIMEOUT).count()));
		m_confirmations.set(StorageHelper::getData(ID_SHOW_CONFIRMATION_DIALOGS) ? 1 : 0);
		m_machinePositionMode.set(StorageHelper::getData(ID_MOVE_MACHINE_POSITION_MODE) ? 1 : 0);
		m_progressSource.set(static_cast<int32_t>(StorageHelper::getData(ID_JOB_PROGRESS_SOURCE)));
		m_connectedMessage.set(StorageHelper::getData(ID_DISPLAY_CONNECTED_MESSAGE) ? 1 : 0);
		m_notificationLevel.set(static_cast<int32_t>(StorageHelper::getData(ID_NOTIFICATION_LEVEL)));
		m_notificationTimeout.set(static_cast<int32_t>(StorageHelper::getData(ID_NOTIFICATION_TIMEOUT).count()));
		m_errorNotificationsStay.set(StorageHelper::getData(ID_NOTIFICATION_AUTO_CLOSE_ERROR) ? 0 : 1);
		m_languageConfirm.set(0);
	}

	void GeneralSettingsPresenter::setting(std::string name,
										   std::string description,
										   std::function<void(float value)> store)
	{
		action(std::move(name),
			   std::move(description),
			   [store = std::move(store)](const Xml::ActionArgs& args)
			   {
				   if (const auto value = args.value())
				   {
					   store(*value);
				   }
			   });
	}

	void GeneralSettingsPresenter::askLanguage(size_t index)
	{
		if (index >= m_languageCodes.size() || m_languageCodes.at(index) == i18n::getCurrentLanguage())
		{
			return;
		}
		LOG_INFO("Language selection changed to: {:s}", m_languageCodes.at(index));
		m_language.set(static_cast<int32_t>(index));
		m_pickedLanguage = m_languageCodes.at(index);
		m_languageConfirmTitle.set(_("settings.language_confirm.title"));
		m_languageConfirmText.set(_("settings.language_confirm.text"));
		m_languageConfirm.set(1);
	}
} // namespace UI

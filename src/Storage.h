/*
 * Storage.h
 *
 *  Created on: 16 Apr 2024
 *      Author: andy
 */

#ifndef JNI_STORAGE_H_
#define JNI_STORAGE_H_

#include <chrono>
#include <concepts>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <termios.h>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
concept StorageKeyValue = std::is_convertible_v<T, nlohmann::json> || std::is_enum_v<T>;

namespace detail
{
	// Primary template declaration (no definition) with optional DefaultProvider type parameter
	template <typename K, StorageKeyValue T, typename DefaultProvider = void>
	struct BaseStorageKey;

	// Default-value specialization (no provider)
	template <typename K, StorageKeyValue T>
		requires std::is_convertible_v<K, std::string_view> && std::is_constructible_v<K, std::string_view>
	struct BaseStorageKey<K, T, void>
	{
		using storage_key_marker = void;
		using value_type = T;
		using default_provider_type = void;
		static constexpr bool has_provider = false;

		consteval BaseStorageKey(std::string_view id_, const T& default_value_) noexcept
			requires std::same_as<K, std::string_view>
			: id(id_)
			, default_value(default_value_)
		{
		}

		constexpr BaseStorageKey(std::string_view id_, const T& default_value_) noexcept
			requires(!std::same_as<K, std::string_view>)
			: id(id_)
			, default_value(default_value_)
		{
		}

		const K id;
		const T default_value;
	};

	// Provider specialization (callable returning T)
	template <typename K, StorageKeyValue T, typename DefaultProvider>
		requires std::is_convertible_v<K, std::string_view> && std::is_constructible_v<K, std::string_view> &&
				 std::invocable<DefaultProvider> && std::is_convertible_v<std::invoke_result_t<DefaultProvider>, T>
	struct BaseStorageKey<K, T, DefaultProvider>
	{
		using storage_key_marker = void;
		using value_type = T;
		using default_provider_type = DefaultProvider;
		static constexpr bool has_provider = true;

		consteval BaseStorageKey(std::string_view id_, DefaultProvider provider_) noexcept
			requires std::same_as<K, std::string_view>
			: id(id_)
			, default_provider(provider_)
		{
		}

		constexpr BaseStorageKey(std::string_view id_, DefaultProvider provider_) noexcept
			requires(!std::same_as<K, std::string_view>)
			: id(id_)
			, default_provider(provider_)
		{
		}

		const K id;
		const DefaultProvider default_provider;
	};

	// Helper to resolve default at compile time based on provider presence
	template <typename K, StorageKeyValue T, typename DefaultProvider>
	[[nodiscard]] constexpr T resolve_default(const BaseStorageKey<K, T, DefaultProvider>& key) noexcept
	{
		if constexpr (std::is_void_v<DefaultProvider>)
		{
			return key.default_value;
		}
		else
		{
			return std::invoke(key.default_provider);
		}
	}
} // namespace detail

template <StorageKeyValue T, typename DefaultProvider = void>
using StorageKey = detail::BaseStorageKey<std::string_view, T, DefaultProvider>;

template <StorageKeyValue T, typename DefaultProvider = void>
using StorageKeyRunTime = detail::BaseStorageKey<std::string, T, DefaultProvider>;

/* Forward declare types */

namespace Comm
{
	enum class UsbMode;
	enum class CommunicationType;
} // namespace Comm
namespace OM
{
	namespace FileSystem
	{
		enum class SortBy;
	}
} // namespace OM
namespace Log
{
	enum class DebugLevel;
}
namespace Units
{
	enum class UnitSystem;
}
enum class ResponseType;

/* Convertors */

namespace std::chrono
{
	void to_json(nlohmann::json& j, const std::chrono::seconds& c);
	void from_json(const nlohmann::json& j, std::chrono::seconds& c);
	void to_json(nlohmann::json& j, const std::chrono::milliseconds& c);
	void from_json(const nlohmann::json& j, std::chrono::milliseconds& c);
} // namespace std::chrono

/* Duet */
extern const StorageKey<std::string_view> ID_DUET_IP_ADDRESS;
extern const StorageKey<std::string_view> ID_DUET_PASSWORD;
extern const StorageKey<Comm::CommunicationType> ID_DUET_COMMUNICATION_TYPE;
extern const StorageKey<std::chrono::milliseconds> ID_DUET_POLL_INTERVAL;
extern const StorageKey<speed_t> ID_DUET_BAUD_RATE;

/* UI */
extern const StorageKey<std::string_view> ID_THEME;
extern const StorageKey<std::string_view> ID_FONT;
extern const StorageKey<std::string_view> ID_ICON_FOLDER;
extern const StorageKey<std::string_view> ID_KEYBOARD_LAYOUT;
extern const StorageKey<bool> ID_UI_ANIMATIONS_ENABLED;

extern const StorageKey<bool> ID_SCREENSAVER_ENABLE;
extern const StorageKey<std::chrono::seconds> ID_SCREENSAVER_TIMEOUT;
extern const StorageKey<std::chrono::milliseconds> ID_NOTIFICATION_TIMEOUT;
extern const StorageKey<ResponseType> ID_NOTIFICATION_LEVEL;
extern const StorageKey<bool> ID_NOTIFICATION_AUTO_CLOSE_ERROR;

extern const StorageKey<bool> ID_UI_CONSOLE_COMMAND_LIST_COLLAPSED;

extern const StorageKey<std::vector<float>, std::vector<float> (*)()> ID_BABYSTEP_AMOUNT;
extern const StorageKey<std::vector<float>, std::vector<float> (*)()> ID_MOVE_DISTANCES;
extern const StorageKey<std::vector<uint32_t>, std::vector<uint32_t> (*)()> ID_MOVE_FEEDRATES;

extern const StorageKey<bool> ID_SHOW_CONFIRMATION_DIALOGS;

/* Multi value selectors */
// these will have the following sub keys {"values", "selected"}
extern const std::string_view ID_MVS_EXTRUSION_FEEDRATES;
extern const std::string_view ID_MVS_EXTRUSION_DISTANCES;

extern const StorageKey<Units::UnitSystem> ID_UNIT_SYSTEM;

/* System */
extern const StorageKey<time_t> ID_UPGRADE_FILE_LAST_MODIFIED;
extern const StorageKey<std::string_view> ID_SYS_LANG_CODE_KEY;
extern const StorageKey<unsigned int> ID_SYS_BRIGHTNESS_KEY;
extern const StorageKey<Comm::UsbMode> ID_USB_MODE;
extern const StorageKey<bool> ID_DISPLAY_CONNECTED_MESSAGE;
extern const StorageKey<bool> ID_ENABLE_ADVANCED_SETTINGS;

/* Debug */
extern const StorageKey<Log::DebugLevel> ID_DEBUG_LEVEL;
extern const StorageKey<std::string_view> ID_LOG_FILE;
extern const StorageKey<bool> ID_ENABLE_UI_LOGGING;
extern const StorageKey<std::chrono::milliseconds> ID_BURNIN_FREQUENCY;
extern const StorageKey<bool> ID_SYSTEM_MONITOR_ENABLED;
extern const StorageKey<bool> ID_ENABLE_SECOND_USB_CHANNEL;

#if DEBUG_BORDERS
extern const StorageKey<bool> ID_DEBUG_BORDERS;
#endif

#if DEVELOPER_MODE
#endif

#endif /* JNI_STORAGE_H_ */

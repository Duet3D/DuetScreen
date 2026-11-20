#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Forward declarations for types used by event payloads to keep this header lightweight
namespace OM
{
	enum class PrinterStatus;
} // namespace OM

enum class ResponseType;

namespace UpgradeHelper
{
	struct UpgradeInfo;
} // namespace UpgradeHelper

namespace NetworkHelper
{
	enum class ConnectResult;
} // namespace NetworkHelper

namespace Log
{
	enum class DebugLevel;
	using log_time_t = std::chrono::system_clock::time_point;
} // namespace Log

namespace UI
{
	class LvObj;
} // namespace UI

namespace Comm
{
	struct FileTransfer;
} // namespace Comm

// Central event registry. Add events here with optional payload types.
#define EVENTS(XX)                                                                                                     \
	XX(Tick)                                                                                                           \
	XX(Connected)                                                                                                      \
	XX(Disconnected)                                                                                                   \
	XX(UpdateAvailable, std::string)                                                                                   \
	XX(GithubUpdateAvailable, std::string)                                                                             \
	XX(UpdateResult, UpgradeHelper::UpgradeInfo)                                                                       \
	XX(UsbDrive, std::string, bool)                                                                                    \
	XX(ObjectModelChanged)                                                                                             \
	XX(ThumbnailData, std::string)                                                                                     \
	XX(FileInfo, std::string)                                                                                          \
	XX(FileTransfer, Comm::FileTransfer)                                                                               \
	XX(Response, ResponseType, std::string)                                                                            \
	XX(ConsoleCommand, std::string)                                                                                    \
	XX(LogMessage, Log::DebugLevel, Log::log_time_t, std::string)                                                      \
	XX(PrinterUniqueId)                                                                                                \
	XX(Status, OM::PrinterStatus)                                                                                      \
	XX(NavigationHomeEnable, bool)                                                                                     \
	XX(NavigationBackEnable, bool)                                                                                     \
	XX(NavigationCurrentScreen, UI::LvObj*)                                                                            \
	XX(Filaments, std::vector<std::string>)                                                                            \
	XX(ThemeChanged)                                                                                                   \
	XX(WifiNetworks)                                                                                                   \
	XX(WifiConnectResult, std::string, NetworkHelper::ConnectResult)

enum class EventType
{
#define XX(name, ...) name,
	EVENTS(XX)
#undef XX
	Null
};

// Primary template (unused, specialized below)
template <EventType E>
struct EventTraits;

// Specializations mapping EventType -> argument tuple
#define XX(name, ...)                                                                                                  \
	template <>                                                                                                        \
	struct EventTraits<EventType::name>                                                                                \
	{                                                                                                                  \
		using tuple_type = std::tuple<__VA_ARGS__>;                                                                    \
	};

EVENTS(XX)
#undef XX

// Null event traits
template <>
struct EventTraits<EventType::Null>
{
	using tuple_type = std::tuple<>;
};

// Concepts to validate handlers and argument lists at compile time
namespace UI
{
	// Check if a callable F is invocable with the types contained in Tuple
	template <typename F, typename Tuple, std::size_t... I>
	constexpr bool invocable_from_tuple_impl(std::index_sequence<I...>)
	{
		return std::is_invocable_v<F&,
								   std::add_lvalue_reference_t<std::add_const_t<std::tuple_element_t<I, Tuple>>>...>;
	}

	template <typename F, typename Tuple>
	concept InvocableFromTuple = invocable_from_tuple_impl<std::remove_reference_t<F>, Tuple>(
		std::make_index_sequence<std::tuple_size_v<Tuple>>{});

	// Check if a parameter pack Args exactly matches an expected tuple of types
	template <typename Expected, typename... Args>
	concept ExactArgsMatch = std::same_as<Expected, std::tuple<std::decay_t<Args>...>>;
} // namespace UI

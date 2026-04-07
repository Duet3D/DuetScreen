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
	struct Alert;
	enum class PrinterStatus;
} // namespace OM

enum class ResponseType;

namespace UpgradeHelper
{
	struct UpgradeInfo;
} // namespace UpgradeHelper

namespace Log
{
	enum class DebugLevel;
	using log_time_t = std::chrono::system_clock::time_point;
} // namespace Log

// Central event registry. Add events here with optional payload types.
#define EVENTS(XX)                                                                                                     \
	XX(Tick)                                                                                                           \
	XX(Connected)                                                                                                      \
	XX(Disconnected)                                                                                                   \
	XX(UpdateAvailable, std::string)                                                                                   \
	XX(GithubUpdateAvailable, std::string)                                                                             \
	XX(UpdateResult, UpgradeHelper::UpgradeInfo)                                                                       \
	XX(FanData)                                                                                                        \
	XX(BedHeaterData)                                                                                                  \
	XX(ChamberHeaterData)                                                                                              \
	XX(HeaterData)                                                                                                     \
	XX(JobFileName, std::string)                                                                                       \
	XX(JobLastFileName, std::string)                                                                                   \
	XX(JobPrintTime)                                                                                                   \
	XX(JobDuration)                                                                                                    \
	XX(JobTimeLeft)                                                                                                    \
	XX(JobWarmupDuration)                                                                                              \
	XX(JobHeight)                                                                                                      \
	XX(JobBuild)                                                                                                       \
	XX(JobCurrentObject)                                                                                               \
	XX(JobObjectData)                                                                                                  \
	XX(ThumbnailData, std::string)                                                                                     \
	XX(AxesData)                                                                                                       \
	XX(ExtruderData)                                                                                                   \
	XX(KinematicsName, std::string)                                                                                    \
	XX(SpeedFactor)                                                                                                    \
	XX(WorkplaceNumber)                                                                                                \
	XX(NoMoveBeforeHoming)                                                                                             \
	XX(PrintingAcceleration, uint32_t)                                                                                 \
	XX(CurrentMoveRequestedSpeed)                                                                                      \
	XX(CurrentMoveTopSpeed)                                                                                            \
	XX(CurrentMoveExtrusionSpeed)                                                                                      \
	XX(MotionSystemData)                                                                                               \
	XX(CompensationFile)                                                                                               \
	XX(Response, ResponseType, std::string)                                                                            \
	XX(LogMessage, Log::DebugLevel, Log::log_time_t, std::string)                                                      \
	XX(AnalogSensorData)                                                                                               \
	XX(EndstopData)                                                                                                    \
	XX(SpindleData)                                                                                                    \
	XX(NetworkName)                                                                                                    \
	XX(PrinterUniqueId)                                                                                                \
	XX(IpAddress, std::string)                                                                                         \
	XX(Status, OM::PrinterStatus)                                                                                      \
	XX(CurrentTool)                                                                                                    \
	XX(Alert, OM::Alert)                                                                                               \
	XX(Time)                                                                                                           \
	XX(ToolData)                                                                                                       \
	XX(ToolHeaterData, size_t)                                                                                         \
	XX(Directories)                                                                                                    \
	XX(NavigationHomeEnable, bool)                                                                                     \
	XX(NavigationBackEnable, bool)                                                                                     \
	XX(Filaments, std::vector<std::string>)                                                                            \
	XX(MachJobs)                                                                                                       \
	XX(MachCurrentJob)                                                                                                 \
	XX(MachNextJob)                                                                                                    \
	XX(MachJobState)                                                                                                   \
	XX(MachJobHistory)

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

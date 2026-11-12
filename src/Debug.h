/*
 * Debug.h
 *
 *  Created on: Nov 19, 2023
 *      Author: Andy Everitt
 */

#pragma once

#include "tracy/Tracy.hpp"
#include "utils/utils.h"
#include <chrono>
#include <fmt/ranges.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

namespace Log
{
	using log_time_t = std::chrono::system_clock::time_point;

	constexpr const char* DebugLevelStrings[] = {
		"settings.severity.verbose",
		"settings.severity.debug",
		"settings.severity.info",
		"settings.severity.warn",
		"settings.severity.error",
		"settings.severity.fatal",
	};

	enum class DebugLevel
	{
		Verbose = 0,
		Debug,
		Info,
		Warn,
		Error,
		Fatal
	};

	void Init();
	void SetDebugLevel(DebugLevel level);
	const DebugLevel& GetDebugLevel();
	void CloseDebugFile();

	void EnableUiLogging(bool enable);
	bool IsUiLoggingEnabled();

	size_t GetThreadId();

	std::shared_ptr<spdlog::logger> GetLogger();

	/**
	 * The logger every LOG_* macro writes to: DuetScreen's once Init() has run, spdlog's default before. It is never
	 * destroyed, so destructors of static objects can still log while the program exits.
	 */
	spdlog::logger* Logger();

	/* This is needed so we get compiler errors if the format string is invalid */
	template <typename... Args>
	void log(spdlog::source_loc loc, spdlog::level::level_enum lvl, fmt::format_string<Args...> fmt, Args&&... args)
	{
		Logger()->log(loc, lvl, std::forward<fmt::format_string<Args...>>(fmt), std::forward<Args>(args)...);
	}
} // namespace Log

/* Need to use macros here so that __FILE_RELPATH__ and __LINE__ are correct */

#define CUSTOM_SPDLOG_LOGGER_CALL(level, ...)                                                                          \
	Log::log(spdlog::source_loc{__FILE_RELPATH__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)

#if DEBUG
#  define LOG_VERBOSE(...) CUSTOM_SPDLOG_LOGGER_CALL(spdlog::level::trace, __VA_ARGS__)
#else
#  define LOG_VERBOSE(...)
#endif

#define LOG_DBG(...) CUSTOM_SPDLOG_LOGGER_CALL(spdlog::level::debug, __VA_ARGS__)
#define LOG_INFO(...) CUSTOM_SPDLOG_LOGGER_CALL(spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...) CUSTOM_SPDLOG_LOGGER_CALL(spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) CUSTOM_SPDLOG_LOGGER_CALL(spdlog::level::err, __VA_ARGS__);
#define LOG_FATAL_THROW(...)                                                                                           \
	CUSTOM_SPDLOG_LOGGER_CALL(spdlog::level::critical, __VA_ARGS__);                                                   \
	spdlog::dump_backtrace();                                                                                          \
	std::abort();

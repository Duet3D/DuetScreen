/*
 * Debug.cpp
 *
 *  Created on: 10 May 2024
 *      Author: Andy Everitt
 */

#include "Debug.h"
#include "BuildDate.h"
#include "Configuration.h"
#include "Storage.h"
#include "UI/Core/Model.h"
#include "utils/StorageHelper.h"
#include "version.h"
#include <ctime>
#include <memory>
#include <spdlog/details/os.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <vector>

// Use global namespace for std
using std::make_shared;
using std::shared_ptr;
using std::vector;

#define LOG_FORMAT_BOLD_START "\033[1m"
#define LOG_FORMAT_BOLD_END "\033[22m"
#define LOG_FORMAT_ITALIC_START "\033[3m"
#define LOG_FORMAT_ITALIC_END "\033[23m"
#define LOG_FORMAT_UNDERLINE_START "\033[4m"
#define LOG_FORMAT_UNDERLINE_END "\033[24m"

#ifdef TRACY_ENABLE
#  define TRACE_LOG_MESSAGES 1
#else
#  define TRACE_LOG_MESSAGES 0
#endif

// TODO enable serverity when tracy supports it in a full release
// Using v0.13.1 because it allows people to use the tracy server on windows from
// https://github.com/wolfpld/tracy/releases assets
#define USE_TRACY_SEVERITY 0 // v0.13.1 doesn't have MessageSeverity enum

#if LOG_TIMESTAMPS
#  define LOG_TIMESTAMP_FMT "[%Y-%m-%d %H:%M:%S.%e] "
#else
#  define LOG_TIMESTAMP_FMT ""
#  define LOG_UI_TIMESTAMP_FMT ""
#endif

#define LOG_CONSOLE_PATTERN                                                                                            \
	"%^" LOG_TIMESTAMP_FMT "[%l] [%t] " LOG_FORMAT_UNDERLINE_START LOG_FORMAT_ITALIC_START                             \
	"%@" LOG_FORMAT_UNDERLINE_END " %!()" LOG_FORMAT_ITALIC_END " %v%$"

#define LOG_FILE_PATTERN LOG_TIMESTAMP_FMT "[%l] [%t] %@ %!() %v"

#define LOG_UI_PATTERN "[%l] %v"

#if USE_TRACY_SEVERITY
#  define LOG_TRACY_PATTERN "[%@ %!()] %v"
#else
#  define LOG_TRACY_PATTERN "[%L] [%@ %!()] %v"
#endif

#if defined(TRACY_ENABLE)
#  include "utils/TracyMemory.h"
#endif

namespace Log
{
#if LV_USE_LOG
	static void lvgl_log_cb(lv_log_level_t level, const char* buf);
#endif

	template <typename Mutex>
	class UiSink : public spdlog::sinks::base_sink<Mutex>
	{
	  protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			ZoneScoped;
			// mutex is locked by base_sink
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
			std::string str(formatted.data(), formatted.size());
			if (Model::isInitialized())
				Model::get().post<EventType::LogMessage>(static_cast<DebugLevel>(msg.level), msg.time, std::move(str));
		}

		void flush_() override {}
	};

	using UiSink_mt = UiSink<std::mutex>;

#if TRACE_LOG_MESSAGES
	template <typename Mutex>
	class TracySink : public spdlog::sinks::base_sink<Mutex>
	{
	  protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			ZoneScoped;
			// mutex is locked by base_sink
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
			uint32_t color;

#  if USE_TRACY_SEVERITY
			tracy::MessageSeverity severity;
#  endif
			switch (msg.level)
			{
			case spdlog::level::trace:
				color = tracy::Color::LightGrey;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Trace;
#  endif
				break;
			case spdlog::level::debug:
				color = tracy::Color::LightBlue;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Debug;
#  endif
				break;
			case spdlog::level::info:
				color = tracy::Color::Green;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Info;
#  endif
				break;
			case spdlog::level::warn:
				color = tracy::Color::Yellow;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Warning;
#  endif
				break;
			case spdlog::level::err:
				color = tracy::Color::Red;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Error;
#  endif
				break;
			case spdlog::level::critical:
				color = tracy::Color::Purple;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Fatal;
#  endif
				break;
			default:
				color = tracy::Color::White;
#  if USE_TRACY_SEVERITY
				severity = tracy::MessageSeverity::Info;
#  endif
				break;
			}

#  if USE_TRACY_SEVERITY
			tracy::Profiler::LogString(
				tracy::MessageSourceType::User, severity, color, TRACY_CALLSTACK, formatted.size(), formatted.data());
#  else
			TracyMessageC(formatted.data(), formatted.size(), color);
#  endif
		}

		void flush_() override {}
	};

	using TracySink_mt = TracySink<std::mutex>;
#endif

	static DebugLevel s_debugLevel = DebugLevel::Info;
	/**
	 * DuetScreen's logger. Deliberately never deleted: static objects (e.g. the USB device) log from their
	 * destructors, which can run after any other static, including spdlog's registry, has been destroyed.
	 */
	static shared_ptr<spdlog::logger>& loggerHolder()
	{
		static auto* s_holder = new shared_ptr<spdlog::logger>();
		return *s_holder;
	}
	static shared_ptr<UiSink_mt> s_uiSink;

	void Init()
	{
		ZoneScoped;
		try
		{
			auto console_sink = make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_pattern(LOG_CONSOLE_PATTERN);

			auto file_sink = make_shared<spdlog::sinks::rotating_file_sink_mt>(
				std::string(StorageHelper::getData(ID_LOG_FILE)), DEFAULT_LOG_FILE_SIZE, DEFAULT_LOG_FILE_COUNT - 1);
			file_sink->set_pattern(LOG_FILE_PATTERN);
			file_sink->set_level(spdlog::level::debug);

#if TRACE_LOG_MESSAGES
			auto tracy_sink = make_shared<TracySink_mt>();
			tracy_sink->set_pattern(LOG_TRACY_PATTERN);
#endif
			spdlog::sinks_init_list sinks{std::move(console_sink),
										  std::move(file_sink)
#if TRACE_LOG_MESSAGES
											  ,
										  std::move(tracy_sink)
#endif
			};
			loggerHolder() = make_shared<spdlog::logger>("duetscreen", sinks);
			loggerHolder()->flush_on(spdlog::level::debug);
			spdlog::set_default_logger(loggerHolder());
			LOG_INFO("\n\n\n----------------------------------------------------------------------------------\n"
					 "Program started\n"
					 "Version: " FIRMWARE_VERSION "\n"
					 "Build date: {} {}\n"
					 "----------------------------------------------------------------------------------\n\n\n",
					 BuildDateText,
					 BuildTimeSuffix);
			SetDebugLevel(StorageHelper::getData(ID_DEBUG_LEVEL));
			EnableUiLogging(StorageHelper::getData(ID_ENABLE_UI_LOGGING));
			spdlog::flush_every(std::chrono::seconds(1));
			spdlog::enable_backtrace(100);

#if LV_USE_LOG
			lv_log_register_print_cb(lvgl_log_cb);
#endif

			LOG_INFO("Logger initialized");

#if defined(TRACY_ENABLE) && TRACY_ENABLE
			// Start lightweight memory plots (RSS and heap) for Tracy in Simulation builds
			TracyMemory::Start();
#endif
		}
		catch (const spdlog::spdlog_ex& ex)
		{
			fprintf(stderr, "Log initialization failed: %s\n", ex.what());
		}
	}

	void SetDebugLevel(DebugLevel level)
	{
		ZoneScoped;
		s_debugLevel = level;
		StorageHelper::setData(ID_DEBUG_LEVEL, level);

		if (!loggerHolder())
			return;

		switch (level)
		{
		case DebugLevel::Verbose:
			loggerHolder()->set_level(spdlog::level::trace);
			break;
		case DebugLevel::Debug:
			loggerHolder()->set_level(spdlog::level::debug);
			break;
		case DebugLevel::Info:
			loggerHolder()->set_level(spdlog::level::info);
			break;
		case DebugLevel::Warn:
			loggerHolder()->set_level(spdlog::level::warn);
			break;
		case DebugLevel::Error:
			loggerHolder()->set_level(spdlog::level::err);
			break;
		case DebugLevel::Fatal:
			loggerHolder()->set_level(spdlog::level::critical);
			break;
		default:
			loggerHolder()->set_level(spdlog::level::info);
		}
	}

	void CloseDebugFile()
	{
		ZoneScoped;
		if (loggerHolder())
		{
			loggerHolder()->flush();
		}
	}

	void EnableUiLogging(bool enable)
	{
		ZoneScoped;
		if ((s_uiSink && enable) || (!s_uiSink && !enable))
			return;

		if (enable)
		{
			s_uiSink = make_shared<UiSink_mt>();
			s_uiSink->set_pattern(LOG_UI_PATTERN);
			s_uiSink->set_level(spdlog::level::info);
			loggerHolder()->sinks().push_back(s_uiSink);
		}
		else
		{
			auto& sinks = loggerHolder()->sinks();
			for (auto it = sinks.begin(); it != sinks.end(); ++it)
			{
				if (*it == s_uiSink)
				{
					sinks.erase(it);
					break;
				}
			}
			s_uiSink.reset();
		}
	}

	bool IsUiLoggingEnabled()
	{
		ZoneScoped;
		return s_uiSink != nullptr;
	}

	size_t GetThreadId()
	{
		ZoneScoped;
		return spdlog::details::os::thread_id();
	}

	const DebugLevel& GetDebugLevel()
	{
		ZoneScoped;
		return s_debugLevel;
	}

	spdlog::logger* Logger()
	{
		const auto& logger = loggerHolder();
		return logger ? logger.get() : spdlog::default_logger_raw();
	}

	shared_ptr<spdlog::logger> GetLogger()
	{
		ZoneScoped;
		return loggerHolder();
	}

#if LV_USE_LOG
	static void lvgl_log_cb(lv_log_level_t level, const char* buf)
	{
		ZoneScoped;
		switch (level)
		{
		case LV_LOG_LEVEL_TRACE:
			LOG_VERBOSE("{:s}", buf);
			break;
		case LV_LOG_LEVEL_INFO:
			LOG_DBG("{:s}", buf);
			break;
		case LV_LOG_LEVEL_USER:
			LOG_INFO("{:s}", buf);
			break;
		case LV_LOG_LEVEL_WARN:
			LOG_WARN("{:s}", buf);
			break;
		case LV_LOG_LEVEL_ERROR:
			LOG_ERROR("{:s}", buf);
			break;
		default:
			break;
		}
	}
#endif
} // namespace Log

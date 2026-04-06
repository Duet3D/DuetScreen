/*
 * Configuration.h
 *
 *  Created on: 15 Mar 2024
 *      Author: andy
 */

#ifndef JNI_CONFIGURATION_H_
#define JNI_CONFIGURATION_H_

#include <chrono>
#include <stdint.h>
#include <string_view>
#include <sys/types.h>
#include <termios.h>

#define UPGRADE_FILE_NAME "DuetScreen"

using namespace std::chrono_literals;

constexpr std::string_view NVS_FOLDER =
#if SIMULATION
	"./var/lib/duetscreen";
#else
	"/var/lib/duetscreen";
#endif

/* Logging */
#if SIMULATION
constexpr std::string_view DEFAULT_LOG_FILE = "DuetScreen.log";
constexpr size_t DEFAULT_LOG_FILE_SIZE = 1024 * 1024 * 10; // 10MB
#else
constexpr std::string_view DEFAULT_LOG_FILE = "/var/log/DuetScreen.log";
constexpr size_t DEFAULT_LOG_FILE_SIZE = 1024 * 1024 * 5; // 5MB
#endif

constexpr size_t DEFAULT_LOG_FILE_COUNT = 3; // 3 files

/* UI */
constexpr std::chrono::milliseconds MODEL_TICK_INTERVAL = 100ms; // Interval to tick the model in milliseconds
constexpr size_t MODEL_TICK_HZ = 1000 / MODEL_TICK_INTERVAL.count();
constexpr std::chrono::seconds DEFAULT_SCREEN_TIMEOUT = 5min;
constexpr std::string_view DEFAULT_ICON_SET = "material";

/* Duet */
constexpr std::string_view DEFAULT_GCODES_PATH = "0:/gcodes";
constexpr std::string_view DEFAULT_MACROS_PATH = "0:/macros";
constexpr std::chrono::milliseconds DEFAULT_PRINTER_POLL_INTERVAL = 250ms;
constexpr std::chrono::milliseconds MIN_PRINTER_POLL_INTERVAL = 100ms;
constexpr std::chrono::milliseconds PRINTER_REQUEST_TIMEOUT = 10000ms;
constexpr std::string_view DEFAULT_IP_ADDRESS = "192.168.0.";
constexpr std::string DEFAULT_UART_PORT = "/dev/ttyS5";
constexpr const int DEFAULT_BAUD_RATE = B115200;
constexpr std::chrono::milliseconds TIME_SYNC_INTERVAL = 10000ms; // Interval to resynchronize time with the Duet
constexpr size_t MAX_UART_UPLOAD_SIZE = 1024;
constexpr std::string_view DEFAULT_FILAMENTS_FILE = "filaments.csv";
constexpr std::string_view DEFAULT_HEIGHTMAPS_FILE = "heightmaps.csv";

/* Thumbnails */
constexpr std::chrono::milliseconds FILE_CACHE_REQUEST_TIMEOUT = 10000ms;
constexpr size_t MAX_THUMBNAIL_CACHE_PIXELS = 64; // Largest pixel width/height thumbnail that is allowed to be cached
constexpr std::chrono::milliseconds BACKGROUND_FILE_CACHE_POLL_INTERVAL = 500ms;
constexpr size_t MAX_FILEINFO_REQUESTS = 1;
constexpr size_t MAX_THUMBNAIL_REQUESTS = 1;

/* Json Decoder */
constexpr size_t MAX_ARRAY_NESTING = 4;
constexpr size_t MAX_JSON_ID_LENGTH = 200;
// 4096 is the largest needed for a Duet in standalone mode. But in
// SBC mode, network responses can be much larger. This is most evident with `rr_thumbnail`
constexpr size_t MAX_JSON_VALUE_LENGTH = 4096 * 20;

/* Network */
// Duet 2 seems to only support 3 concurrent connections. We need 1 connection for synchronous requests, so we can
// only have 2 threads.
constexpr size_t MAX_THREAD_POOL_SIZE = 2;
constexpr uint16_t HTTP_TIMEOUT = 5; // seconds

/* Object Model */
constexpr size_t MAX_TOTAL_AXES = 15; // This needs to be kept in sync with the maximum in RRF
constexpr size_t MAX_MOTION_SYSTEMS = 2;
constexpr size_t MAX_EXTRUDERS_PER_TOOL = 8;
constexpr size_t MAX_HEATERS_PER_TOOL = 8;
constexpr unsigned int MAX_TOOL_NAME_LENGTH = 50;
constexpr unsigned int MAX_FILAMENT_NAME_LENGTH = 100;
constexpr unsigned int MAX_FILENAME_LENGTH = 255;
constexpr unsigned int MAX_SLOTS = 32;
constexpr unsigned int MAX_FANS = 12;
constexpr unsigned int MAX_HEATERS = 32;
constexpr unsigned int MAX_SENSORS = 32;
constexpr unsigned int MAX_ENDSTOPS = 20;
constexpr size_t MAX_TRACKED_OBJECTS = 64;
constexpr size_t MAX_REPORTED_AXES =
	5; // RRF only reports 5 axes in the `move` object, need to request `move.axes` to get the rest

/* Console */
constexpr unsigned int MAX_COMMAND_LENGTH = 50;
constexpr unsigned int MAX_RESPONSE_LINES = 200;

/* Misc UI */
constexpr std::string_view DEFAULT_LANGUAGE_CODE = "en-GB";

/* Alert */
constexpr size_t ALERT_TEXT_LENGTH = 165;		 // maximum characters in the alert text
constexpr size_t ALERT_TITLE_LENGTH = 50;		 // maximum characters in the alert title
constexpr size_t ALERT_RESPONSE_LENGTH = 50;	 // maximum characters in the alert response
constexpr size_t ALERT_MAX_CHOICES = 40;		 // maximum number of choices in the alert
constexpr size_t ALERT_CHOICES_TEXT_LENGTH = 50; // maximum characters in the alert choice text

#endif /* JNI_CONFIGURATION_H_ */

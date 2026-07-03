/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "Comm/Communication.h"
#include "Comm/Usb.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Hardware/Reset.h"
#include "Hardware/Usb.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Styles/Font.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/HardwareTest/HardwareTest.h"
#include "glob.h"
#include "hv/requests.h"
#include "i18n/i18n.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/core/lv_global.h"
#include "tracy/Tracy.hpp"
#include "utils/DisplayHelper.h"
#include "utils/GpioHelper.h"
#include "utils/NetworkHelper.h"
#include "utils/StorageHelper.h"
#include "utils/UpgradeHelper.h"
#include <filesystem>
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <unistd.h>

#if LV_USE_OS == LV_OS_PTHREAD
#  include <pthread.h>
#endif

#if T113
#elif SIMULATION
#endif

/*********************
 *      DEFINES
 *********************/
#define SET_THREAD_PRIORITY 0

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_display_t* hal_init(int32_t w, int32_t h);
static void http_test();
static int usb_test();
static int set_thread_priority(pthread_t thread_id, int policy, int priority);

/**********************
 *  STATIC VARIABLES
 **********************/
static std::thread s_responseThread;
static std::thread s_requestThread;
static std::thread s_thumbnailThread;
static std::thread s_internetMonitorThread;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *      VARIABLES
 **********************/

int main(int argc, char** argv)
{
	(void)argc; /*Unused*/
	(void)argv; /*Unused*/

#if SET_THREAD_PRIORITY
	set_thread_priority(pthread_self(), SCHED_OTHER, 100);
#endif

	// Initialise
	{
		ZoneScopedN("lv_init");
		lv_init();
	}

	Model::get(); // Initialize the model instance, this creates the subscribers
	StorageHelper::load();
	Log::Init();

	// LVGL thread needs access to both the UI and Model mutexes. It is the only thread allowed to take both otherwise
	// deadlocks can occur
	DeadlockDetector::getInstance().allowThreadToTakeMultipleLocks(Log::GetThreadId(), true);

	/*Initialize LVGL*/
	{
		ZoneScopedN("FontManager::init");
		UI::FontManager::init();
	}
	{
		ZoneScopedN("i18n::init");
		i18n::init();
	}

	{
		ZoneScopedN("Comm::init");
		Comm::init();
		Comm::DUET.Init();
	}

	/*Initialize the HAL (display, input devices, tick) for LVGL*/
	lv_display_t* display = hal_init(1024, 600);

	DisplayHelper::setBrightness(StorageHelper::getData(ID_SYS_BRIGHTNESS_KEY));
	{
		ZoneScopedN("Themes::init");
		UI::Themes::init(display);
	}

	DisplayHelper::setRotation(StorageHelper::getData(ID_DISPLAY_ROTATION));

	UI::HomeView& home = UI::HomeView::instance();
	home.show();

#if HARDWARE_TEST
	UI::HardwareTest hw_test;
	hw_test.show(true);
#endif

	Model::get().startEventLoop();

	USB::UsbMonitor::getInstance().registerCallback(
		[](const std::string& path, bool mounted)
		{
			ZoneScopedN("Upgrade USB Callback");
			if (mounted)
			{
				LOG_INFO("USB drive mounted: {:s}", path.c_str());
				std::string upgradeFilePath = path + "/DuetScreen.tar.gz";
				if (!std::filesystem::exists(upgradeFilePath))
				{
					return;
				}

				struct stat file_stat;
				if (stat(upgradeFilePath.c_str(), &file_stat) != 0)
				{
					LOG_ERROR("Error getting file stats for {:s}", upgradeFilePath.c_str());
					return;
				}

				time_t lastModified = file_stat.st_mtime;
				time_t savedModified = StorageHelper::getData(ID_UPGRADE_FILE_LAST_MODIFIED);

				if (lastModified == savedModified)
				{
					return;
				}

				Model::get().post<EventType::UpdateAvailable>(upgradeFilePath);
			}
		});
	USB::UsbMonitor::getInstance().startMonitoring();
	UpgradeHelper::startMonitoringUpgradeStatus();

	s_internetMonitorThread = std::thread(
		[]()
		{
			tracy::SetThreadName("Internet Monitor Thread");
			bool wasOnline = false;
			while (1)
			{
				const bool isOnline = !NetworkHelper::getIpAddress().empty();
				if (isOnline && !wasOnline)
				{
					LOG_INFO("Internet connection detected, checking for newer DuetScreen release");
					UpgradeHelper::checkForUpdate().transform(
						[](const std::string& tag)
						{
							Model::get().post<EventType::GithubUpdateAvailable>(tag);
							return tag;
						});
				}
				wasOnline = isOnline;
				std::this_thread::sleep_for(std::chrono::seconds(5));
			}
		});

	// Create a thread to handle requesting data from Duet
#if MULTITHREADED
	s_requestThread = std::thread(
		[]()
		{
			tracy::SetThreadName("Request Thread");
#  if SET_THREAD_PRIORITY
			// Set high priority for request thread
			set_thread_priority(pthread_self(), SCHED_RR, 90);
#  endif

			while (1)
			{
				// Request next section of the OM
				std::chrono::milliseconds delay = Model::get().requestNewData();
				std::this_thread::sleep_for(delay);
			}
		});

	s_thumbnailThread = std::thread(
		[]()
		{
			tracy::SetThreadName("FileInfo Cache Thread");
#  if SET_THREAD_PRIORITY
			// Set low priority for thumbnail thread
			set_thread_priority(pthread_self(), SCHED_RR, 70);
#  endif

			while (1)
			{
				FILEINFO_CACHE->Spin();
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		});
#endif

	// Screensaver task
	DisplayHelper::setScreenSaverBrightness(0);
	lv_timer_t* screensaver_timer = lv_timer_create(
		[](lv_timer_t* timer)
		{
			ZoneScopedN("Screensaver Timer");
			static bool screensaver_enabled = false;
#if BURNIN_TEST
			static bool first_run = true;
			static lv_timer_t* burnin_timer = lv_timer_create(
				[](lv_timer_t* timer)
				{
					static size_t index = 0;
					auto& home = UI::HomeView::instance();
					static const std::function<void()> cbs[] = {
						[&]()
						{
							UI::home();
							home.getDashboard().showJobsTab();
						},
						[&]()
						{
							UI::home();
							home.getDashboard().showStatusTab();
						},
						[&]()
						{
							UI::openScreen(&home.getControlView(), true);
							home.getControlView().showMoveView();
						},
						[&]() { home.getControlView().showTemperatureView(); },
						[&]() { home.getControlView().showHeightmapView(); },
						[&]() { home.getControlView().showFanView(); },
						[&]()
						{
							UI::openScreen(&home.getFileView(), true);
							home.getFileView().setActiveTab(0);
						},
						[&]() { home.getFileView().setActiveTab(1); },
						[&]() { UI::openScreen(&home.getConsoleView(), true); },
#  if SIDE_BAR_APP_DRAWER
						[&]() { UI::openScreen(&home.getMoveView(), true); },
						[&]() { UI::openScreen(&home.getTemperatureView(), true); },
						[&]() { UI::openScreen(&home.getFineTuneView(), true); },
						[&]() { UI::openScreen(&home.getHeightmapView(), true); },
#  endif
						[&]()
						{
							UI::openScreen(&home.getSettingsView(), true);
							home.getSettingsView().showGeneralSettings();
						},
						[&]() { home.getSettingsView().showConnectionSettings(); },
						[&]() { home.getSettingsView().showDisplaySettings(); },
						[&]() { home.getSettingsView().showDeveloperSettings(); },
					};

					std::invoke(cbs[index]);
					index = (index + 1) % std::size(cbs);
				},
				StorageHelper::getData(ID_BURNIN_FREQUENCY),
				NULL);
			if (first_run)
			{
				lv_timer_pause(burnin_timer);
				first_run = false;
			}
#endif
			std::chrono::milliseconds inactive_time(lv_display_get_inactive_time(NULL));
			const auto timeout = StorageHelper::getData(ID_SCREENSAVER_TIMEOUT);
			const OM::PrinterStatus printerStatus = OM::GetStatus();

			/**
			 * Screen saver will not activate while the printer is `busy` since this is likely when the user will be
			 * interacting with the screen or DWC.
			 *
			 * The status will also be `busy` when a `M291` modal is open.
			 *
			 * It is currently intentional that the screen saver can activate while the status is one of the printing
			 * states since the printer may be in a location where having the screen on full brightness is undesirable,
			 * and the user can easily disable the screen saver in settings if this isn't desired behaviour. This may be
			 * revisited in the future if it is found that users want the screen saver to be disabled during printing as
			 * well.
			 */
			if (timeout > std::chrono::seconds(0) && inactive_time > timeout &&
				(printerStatus != OM::PrinterStatus::busy))
			{
				if (!screensaver_enabled)
				{
					LOG_INFO("Screensaver timeout reached");
#if BURNIN_TEST
					lv_timer_resume(burnin_timer);
#else
					DisplayHelper::enableScreenSaver(true);
#endif
					screensaver_enabled = true;
				}
			}
			else
			{
				if (screensaver_enabled)
				{
					LOG_INFO("Screensaver timeout cancelled");
#if BURNIN_TEST
					lv_timer_pause(burnin_timer);
#else
					DisplayHelper::enableScreenSaver(false);
#endif
					screensaver_enabled = false;
				}
			}
		},
		250, // Timer period in milliseconds
		NULL);

	lv_timer_create(
		[](lv_timer_t* timer)
		{
			ZoneScopedN("Watchdog Timer");
			if (system("touch /tmp/duetscreen-watchdog") != 0)
			{
				LOG_ERROR("Failed to update watchdog timestamp");
			}
		},
		1000,
		NULL);

	// Try to set UI thread to real-time priority first
	if (set_thread_priority(pthread_self(), SCHED_FIFO, sched_get_priority_max(SCHED_FIFO)) != 0)
	{
		// If real-time priority fails, fall back to highest normal priority
		LOG_WARN("Failed to set real-time priority, falling back to SCHED_OTHER");
		set_thread_priority(pthread_self(), SCHED_OTHER, sched_get_priority_max(SCHED_OTHER));
	}

	const auto targetInterval = std::chrono::milliseconds(5);
	auto nextRunTime = std::chrono::steady_clock::now();

	while (1)
	{
		{
			UI_LOCK();
			ZoneScopedN("lv_timer_handler");
			lv_timer_handler();
			FrameMark;
		}

		nextRunTime += targetInterval;
		std::this_thread::sleep_until(nextRunTime);
	}

	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static const char* getenv_default(const char* name, const char* dflt)
{
	ZoneScoped;
	return getenv(name) ?: dflt;
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static lv_display_t* hal_init(int32_t w, int32_t h)
{
	ZoneScoped;
	LOG_INFO("Initialising display");

	lv_display_t* disp = NULL;
	lv_group_set_default(lv_group_create());

#if LV_USE_LINUX_FBDEV
	const char* device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");

	{
		ZoneScopedN("Framebuffer Initialization");
		disp = lv_linux_fbdev_create();
		lv_display_set_resolution(disp, w, h);
	}

#  if LV_USE_EVDEV
	{
		ZoneScopedN("Input Device Initialization");
		const char* input_device = getenv_default("LV_LINUX_EVDEV_POINTER_DEVICE", "/dev/input/event1");
		lv_indev_t* touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, input_device);
		lv_indev_set_display(touch, disp);

		lv_evdev_discovery_start(
			[](lv_indev_t* indev, lv_evdev_type_t type, void* user_data)
			{
				ZoneScopedN("EVDEV Discovery Callback");
				LOG_INFO("New evdev device discovered: type {}, user_data {}",
						 magic_enum::enum_name(type),
						 (uintptr_t)user_data);
				lv_display_t* disp = (lv_display_t*)user_data;
				if (type == LV_EVDEV_TYPE_KEY)
				{
					lv_indev_set_display(indev, disp);
					lv_indev_set_group(indev, lv_group_get_default());

#	if LV_EVDEV_XKB
					// Apply stored keyboard layout
					{
						auto layoutCode = StorageHelper::getData(ID_KEYBOARD_LAYOUT);
						struct xkb_rule_names names = {.rules = nullptr,
													   .model = "pc105",
													   .layout = layoutCode.data(),
													   .variant = nullptr,
													   .options = nullptr};
						lv_evdev_set_keymap(indev, names);
						LOG_INFO("Applied keyboard layout: {}", layoutCode);
					}
#	endif

					lv_indev_add_event_cb(
						indev,
						[](lv_event_t* e)
						{
							lv_event_code_t code = lv_event_get_code(e);
							[[maybe_unused]] const auto key =
								lv_indev_get_key(static_cast<lv_indev_t*>(lv_event_get_current_target(e)));
							return;
						},
						LV_EVENT_KEY,
						nullptr);
				}
				if (type == LV_EVDEV_TYPE_REL)
				{
					{
						ZoneScopedN("Mouse Cursor Setup");
						LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
						lv_obj_t* cursor_obj;
						cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
						lv_obj_set_name(cursor_obj, "mouse_cursor");
						lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
						lv_indev_set_cursor(indev, cursor_obj);			  /*Connect the image  object to the driver*/
						lv_indev_add_event_cb(
							indev,
							[](lv_event_t* e)
							{
								ZoneScopedN("Mouse Cursor Cleanup");
								LOG_INFO("Input device deleted, cleaning up cursor");
								lv_obj_t* cursor =
									lv_indev_get_cursor(static_cast<lv_indev_t*>(lv_event_get_current_target(e)));
								if (cursor)
								{
									lv_obj_delete(cursor);
								}
							},
							LV_EVENT_DELETE,
							nullptr);
						lv_obj_add_event_cb(
							cursor_obj,
							[](lv_event_t* e)
							{
								ZoneScopedN("Mouse Activity Callback");
								lv_display_trigger_activity(NULL); // Reset screensaver timer on mouse activity
							},
							LV_EVENT_STYLE_CHANGED, /* XY pos counts as a style change */
							nullptr);
					}
					lv_indev_set_display(indev, disp);
					lv_indev_set_group(indev, lv_group_get_default());
				}
			},
			disp);
	}
#  endif

	lv_linux_fbdev_set_file(disp, device);

#elif LV_USE_SDL

	lv_indev_t* mouse = NULL;

	{
		ZoneScopedN("SDL Window Creation");
		disp = lv_sdl_window_create(w, h);
		lv_obj_set_name(lv_screen_active(), "screen_active");
	}

	{
		ZoneScopedN("SDL Input Devices Creation");
		mouse = lv_sdl_mouse_create();
		lv_indev_set_group(mouse, lv_group_get_default());
		lv_indev_set_display(mouse, disp);
		lv_display_set_default(disp);
	}

	{
		ZoneScopedN("Mouse Cursor Setup");
		LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
		lv_obj_t* cursor_obj;
		cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
		lv_obj_set_name(cursor_obj, "mouse_cursor");
		lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
		lv_indev_set_cursor(mouse, cursor_obj);			  /*Connect the image  object to the driver*/
	}

	{
		ZoneScopedN("SDL Mousewheel Creation");
		lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
		lv_indev_set_display(mousewheel, disp);
		lv_indev_set_group(mousewheel, lv_group_get_default());
	}

	{
		ZoneScopedN("SDL Keyboard Creation");
		lv_indev_t* kb = lv_sdl_keyboard_create();
		lv_indev_set_display(kb, disp);
		lv_indev_set_group(kb, lv_group_get_default());
	}

#else
#  error Unsupported configuration
#endif

#if LV_USE_SYSMON
	{
		ZoneScopedN("System Monitor Setup");
		const bool sysmon_enabled = StorageHelper::getData(ID_SYSTEM_MONITOR_ENABLED);
#  if LV_USE_PERF_MONITOR
		if (sysmon_enabled)
			lv_sysmon_show_performance(NULL);
		else
			lv_sysmon_hide_performance(NULL);
#  endif
#  if LV_USE_MEM_MONITOR
		if (sysmon_enabled)
			lv_sysmon_show_memory(NULL);
		else
			lv_sysmon_hide_memory(NULL);
#  endif
	}
#endif

	return disp;
}

int set_thread_priority(pthread_t thread_id, int policy, int priority)
{
	ZoneScoped;
#ifndef __APPLE__
	sched_param sch;
	int current_policy;
	pthread_getschedparam(thread_id, &current_policy, &sch);
	sch.sched_priority = priority;
	int ret = pthread_setschedparam(thread_id, policy, &sch);
	if (ret != 0)
	{
		LOG_WARN("Failed to set thread priority for thread {}, err {} '{}'", thread_id, ret, strerror(ret));
		return -1;
	}
#endif
	return 0;
}

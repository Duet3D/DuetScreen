/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
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
#include "utils/DisplayHelper.h"
#include "utils/GpioHelper.h"
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
#elif LV_USE_OS == LV_OS_FREERTOS
#  include "freertos_main.h"
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
void lvgl_log_cb(lv_log_level_t level, const char* buf);
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

	lv_init();

	// Initialise
	StorageHelper::load();
	Log::Init();

	// LVGL thread needs access to both the UI and Model mutexes. It is the only thread allowed to take both otherwise
	// deadlocks can occur
	DeadlockDetector::getInstance().allowThreadToTakeMultipleLocks(Log::GetThreadId(), true);

/*Initialize LVGL*/
#if LV_USE_LOG
	lv_log_register_print_cb(lvgl_log_cb);
#endif
	UI::FontManager::init();
	i18n::init();

	Model::get(); // Initialize the model instance, this creates the subscribers

	Comm::init();
	Comm::DUET.Init();

	/*Initialize the HAL (display, input devices, tick) for LVGL*/
	lv_display_t* display = hal_init(1024, 600);

	DisplayHelper::setBrightness(StorageHelper::getData(ID_SYS_BRIGHTNESS_KEY, 100u));
	UI::Themes::init(display);

	// lv_display_set_rotation(display, LV_DISP_ROTATION_180);
#if 1
	UI::Screen screen("screen");
	UI::BabyStep obj("print_info", screen);

	obj.setSize(LV_PCT(30), LV_PCT(40));
#else
	UI::HomeView& home = UI::HomeView::instance();
	home.show();
#endif

#if HARDWARE_TEST
	UI::HardwareTest hw_test;
	hw_test.show(true);
#endif

	Model::get().startEventLoop();

	USB::UsbMonitor::getInstance().registerCallback(
		[](const std::string& path, bool mounted)
		{
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
				time_t savedModified = StorageHelper::getData(ID_UPGRADE_FILE_LAST_MODIFIED, 0);

				if (lastModified == savedModified)
				{
					return;
				}

				Model::get().post<EventType::UpdateAvailable>(upgradeFilePath);
			}
		});
	USB::UsbMonitor::getInstance().startMonitoring();

	// Create a thread to handle requesting data from Duet
#if MULTITHREADED
	s_requestThread = std::thread(
		[]()
		{
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
			static bool screensaver_enabled = false;
#if BURNIN_TEST
			static bool first_run = true;
			static lv_timer_t* burnin_timer = lv_timer_create(
				[](lv_timer_t* timer)
				{
					static size_t screen_index = 0;
					auto& home = UI::HomeView::instance();
					static const std::vector<UI::LvObj*> screens{nullptr,
																 nullptr,
																 &home.getConsoleView(),
																 &home.getMacroView(),
#  if SIDE_BAR_APP_DRAWER
																 &home.getMoveView(),
																 &home.getTemperatureView(),
																 &home.getFineTuneView(),
																 &home.getHeightmapView(),
#  endif
																 &home.getSettingsView()};

					auto screen = screens[screen_index];
					if (screen == nullptr)
					{
						if (screen_index == 0)
						{
							UI::home();
							home.getDashboard().showJobsTab();
						}
						else if (screen_index == 1)
						{
							home.getDashboard().showStatusTab();
						}
					}
					else
					{
						UI::openScreen(screen, true);
					}

					screen_index = (screen_index + 1) % std::size(screens);
				},
				StorageHelper::getData(ID_BURNIN_FREQUENCY, 2000),
				NULL);
			if (first_run)
			{
				lv_timer_pause(burnin_timer);
				first_run = false;
			}
#endif
			uint32_t inactive_time = lv_display_get_inactive_time(NULL);
			uint32_t timeout = StorageHelper::getData(ID_SCREENSAVER_TIMEOUT, DEFAULT_SCREEN_TIMEOUT);
			if (timeout > 0 && inactive_time > timeout)
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
		1000, // Timer period in milliseconds
		NULL);

	lv_timer_create(
		[](lv_timer_t* timer)
		{
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
			lv_timer_handler();
		}

		nextRunTime += targetInterval;
		std::this_thread::sleep_until(nextRunTime);
	}

	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

void lvgl_log_cb(lv_log_level_t level, const char* buf)
{
	switch (level)
	{
	case LV_LOG_LEVEL_TRACE:
		LOG_VERBOSE("{:s}", buf);
		break;
	case LV_LOG_LEVEL_INFO:
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

static const char* getenv_default(const char* name, const char* dflt)
{
	return getenv(name) ?: dflt;
}

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static lv_display_t* hal_init(int32_t w, int32_t h)
{
	LOG_INFO("Initialising display");
#if LV_USE_LINUX_FBDEV
	const char* device = getenv_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
	lv_display_t* disp = lv_linux_fbdev_create();
	lv_display_set_resolution(disp, w, h);

#  if LV_USE_EVDEV
	const char* input_device = getenv_default("LV_LINUX_EVDEV_POINTER_DEVICE", "/dev/input/event1");
	lv_indev_t* touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, input_device);
	lv_indev_set_display(touch, disp);
#  endif

	lv_linux_fbdev_set_file(disp, device);

#elif LV_USE_SDL

	lv_group_set_default(lv_group_create());

	lv_display_t* disp = lv_sdl_window_create(w, h);
	lv_obj_set_name(lv_screen_active(), "screen_active");

	lv_indev_t* mouse = lv_sdl_mouse_create();
	lv_indev_set_group(mouse, lv_group_get_default());
	lv_indev_set_display(mouse, disp);
	lv_display_set_default(disp);

	LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
	lv_obj_t* cursor_obj;
	cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
	lv_obj_set_name(cursor_obj, "mouse_cursor");
	lv_image_set_src(cursor_obj, &mouse_cursor_icon); /*Set the image source*/
	lv_indev_set_cursor(mouse, cursor_obj);			  /*Connect the image  object to the driver*/

	lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
	lv_indev_set_display(mousewheel, disp);
	lv_indev_set_group(mousewheel, lv_group_get_default());

	lv_indev_t* kb = lv_sdl_keyboard_create();
	lv_indev_set_display(kb, disp);
	lv_indev_set_group(kb, lv_group_get_default());

#else
#  error Unsupported configuration
#endif
	return disp;
}

int set_thread_priority(pthread_t thread_id, int policy, int priority)
{
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

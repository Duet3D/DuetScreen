/*
 * UiTestSuite.cpp
 *
 *  Created on: 2025-08-14
 *      Author: Andy Everitt
 */

#include "UiTestSuite.h"
#include "Configuration.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "i18n/i18n.h"
#include "test_utils/utils.h"
#include "utils/StorageHelper.h"

UiTestSuiteInner::UiTestSuiteInner()
	: TestSuite()
{
	ZoneScoped;
	/* Run at start of each test */
	LOG_INFO("Setting up UI test");
	lv_init();

	Log::Init();

#if LV_USE_PROFILER && LV_USE_PROFILER_BUILTIN
	/* Disable profiler, to reduce redundant profiler log printing  */
	lv_profiler_builtin_set_enable(false);
#endif

	lv_display_t* display = lv_test_display_create(HOR_RES, VER_RES);
	lv_test_indev_create_all();

#if LV_USE_GESTURE_RECOGNITION
	lv_test_indev_gesture_create();
#endif

#if LV_USE_SYSMON
#  if LV_USE_MEM_MONITOR
	lv_sysmon_hide_memory(NULL);
#  endif
#  if LV_USE_PERF_MONITOR
	lv_sysmon_hide_performance(NULL);
#  endif
#endif

	UI::FontManager::init();

	i18n::init();
	i18n::setLanguage(DEFAULT_LANGUAGE_CODE);

	StorageHelper::setData(ID_UI_ANIMATIONS_ENABLED, false);

	UI::Themes::init(display);
#if DEBUG_BORDERS
	// UI::Themes::showDebugBorders(lv_screen_active(), true);
#endif

	lv_obj_set_style_pad_all(lv_screen_active(), 0, 0);
	lv_obj_set_name(lv_screen_active(), "screen_active");
}

UiTestSuiteInner::~UiTestSuiteInner()
{
	ZoneScoped;
	/* Run at end of each test */
	// Cleanup
	LOG_INFO("Tearing down UI test");
	UI::Themes::deinit();
	lv_deinit();
}

UiTestSuite::UiTestSuite()
	: UiTestSuiteInner()
{
	ZoneScoped;
	screen.setSize(LV_PCT(100), LV_PCT(100));
	screen.setStylePad(0);
	screen.setFlexFlow(LV_FLEX_FLOW_COLUMN_WRAP);
}

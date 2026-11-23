/*
 * utils.h
 *
 *  Created on: 2025-08-14
 *      Author: Andy Everitt
 */

#pragma once

#include "Debug.h"
#include "UI/Core/Model.h"
#include "lvgl/lvgl.h"
#include <gtest/gtest.h>

#define HOR_RES 1024
#define VER_RES 600

#if LV_COLOR_DEPTH != 32
#  define EXPECT_EQUAL_PICTURE(path) TEST_IGNORE_MESSAGE("Requires LV_COLOR_DEPTH 32");
#  define EXPECT_EQUAL_SCREENSHOT(path) TEST_IGNORE_MESSAGE("Requires LV_COLOR_DEPTH 32");
#else
/**
 * Compare the display with reference image `path` (under tests/ref_imgs/), whatever the display's size. A missing
 * reference is created; a picture that differs from it, in its pixels or its size, is saved beside it as `_err`, for
 * run_tests.py to review. Use EXPECT_EQUAL_PICTURE, which lets Model events through and holds the UI lock first.
 * @return whether it matches
 */
bool comparePicture(const char* path);

/*
 * Compare the display with reference image `path` (see comparePicture()). For a picture of part of the screen, cut
 * the display down to it first (lv_display_set_resolution()).
 */
#  define EXPECT_EQUAL_PICTURE(path)                                                                                   \
	  {                                                                                                                \
		  Model::get().startEventLoop();                                                                               \
		  Model::get().stopEventLoop();                                                                                \
		  UI_LOCK();                                                                                                   \
		  EXPECT_TRUE(comparePicture(path)) << (path);                                                                 \
	  }

/* Compare the whole screen, at the screen's resolution, with reference image `path` (see EXPECT_EQUAL_PICTURE) */
#  define EXPECT_EQUAL_SCREENSHOT(path)                                                                                \
	  if (LV_HOR_RES != HOR_RES || LV_VER_RES != VER_RES)                                                              \
	  {                                                                                                                \
		  LOG_ERROR("Requires {:d}x{:d} resolution", HOR_RES, VER_RES);                                                \
	  }                                                                                                                \
	  else                                                                                                             \
		  EXPECT_EQUAL_PICTURE(path)
#endif

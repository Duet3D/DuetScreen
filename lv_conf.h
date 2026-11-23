/**
 * @file lv_conf.h
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/**
 * This file only contains the parameters which have been changed from their defaults for the DuetScreen project.
 * This makes upgrading lvgl easier as it avoids conflicts when options are changed in the default lv_conf_template.h
 * file.
 *
 * See libraries/lvgl/lv_conf_template.h for the default configuration options.
 *
 * @note lv_conf_template.h is used to created libraries/lvgl/src/lv_conf_internal.h which includes this file and sets
 * all the default options.
 */

/* We use our own OS abstraction layer */
#define LV_USE_OS LV_OS_NONE

/* Required to make gradients look correct on our display */
#define LV_COLOR_DEPTH 32

/* 20MB of memory for LVGL, mostly this is for the unit tests but it doesn't hurt to have it here too. Can be reduced in
 * the future if required */
#define LV_MEM_SIZE (20 * 1024 * 1024U)
#define LV_DRAW_THREAD_STACK_SIZE (32 * 1024U)

/* 100fps refresh rate. Since thread priority is not guaranteed this means even when the fps drops due to other
 * computation, we still hit >60fps */
#define LV_DEF_REFR_PERIOD 10

/* Abort on failed asserts. Default behaviour is to enter an infinite loop which is hard to detect and recover from */
#define LV_ASSERT_HANDLER_INCLUDE "lv_assert_handler_include.h"
#define LV_ASSERT_HANDLER abort();

/* Enable object names which are useful for debugging and unit tests. Also allows the LvObj::getChildByName() API.
 * Also required by the XML UI engine, which addresses everything it builds by name. */
#define LV_USE_OBJ_NAME 1

/* XML UI (helix-xml, see libraries/helix-xml.cmake and src/UI/Xml). The translation module lets XML `translation_tag`s
 * resolve through DuetScreen's i18n tables.
 * LV_USE_FLOAT (float subjects) is deliberately off: it also turns lv_value_precise_t into float across all of LVGL's
 * drawing code, which existing widgets are not written for. Bind integers (e.g. percent, tenths) or strings instead. */
#define LV_USE_XML 1
#define LV_USE_TRANSLATION 1

/* Object style caching */
#define LV_OBJ_STYLE_CACHE 1

/* We use our own theme */
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_MONO 0

/* Fonts */
#define LV_USE_FONT_MANAGER 1
#define LV_USE_FREETYPE 1
#define LV_FREETYPE_CACHE_FT_GLYPH_CNT 2048

/* File system support */
#define LV_FS_DEFAULT_DRIVER_LETTER 'A'
#define LV_USE_FS_STDIO 1
#define LV_FS_STDIO_LETTER 'A'

/* Media support */
#define LV_USE_LODEPNG 1
#define LV_USE_LIBPNG 1
#define LV_USE_BMP 1
#define LV_USE_FFMPEG 1 /* Needed to render the QOI (converted to BMP) thumbnails */

/* Don't build unnecessary components */
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0

/* Logging */
#define LV_LOG_USE_LEVEL 0

/* Profiling */
#if defined LV_USE_PROFILER && LV_USE_PROFILER
#  define LV_PROFILER_STYLE 1
#  define LV_PROFILER_BEGIN LvZoneStart
#  define LV_PROFILER_END LvZoneEnd

#  define LV_PROFILER_BEGIN_TAG(tag) LvZoneStartTag(tag)
#  define LV_PROFILER_END_TAG(tag) LvZoneEndTag(tag)
#endif

#endif /*LV_CONF_H*/

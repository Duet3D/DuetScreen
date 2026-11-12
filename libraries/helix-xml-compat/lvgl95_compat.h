/*
 * Force-included into every helix-xml translation unit (see helix-xml.cmake).
 *
 * helix-xml targets LVGL 9.6. These are the 9.6 names it uses that DuetScreen's
 * LVGL 9.5 fork spells differently. Delete this file, and the include directory
 * next to it, when LVGL is upgraded to 9.6.
 */
#pragma once

#include <lvgl.h>

#if LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR < 6
/* 9.6 renamed lv_observer_remove() to lv_observer_delete(). */
static inline void lv_observer_delete(lv_observer_t * observer)
{
    lv_observer_remove(observer);
}
#endif

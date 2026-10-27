/**
 * @file lv_conf.h
 * LVGL configuration. Only settings which differ from the LVGL defaults are
 * listed here, see lv_conf_template.h in LVGL for the full set of options.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* The ESP-IDF configuration, for platform specific settings (ESP_PLATFORM is not defined
 * for LVGL, since it's built as a regular CMake library and not as an IDF component) */
#if __has_include("sdkconfig.h")
    #include "sdkconfig.h"
#endif

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

/*========================
 * RENDERING CONFIGURATION
 *========================*/

#define LV_DRAW_SW_SUPPORT_RGB565A8     0
#define LV_DRAW_SW_SUPPORT_RGB888       0
#define LV_DRAW_SW_SUPPORT_XRGB8888     0
#define LV_DRAW_SW_SUPPORT_L8           0
#define LV_DRAW_SW_SUPPORT_AL88         0
#define LV_DRAW_SW_SUPPORT_A8           0
#define LV_DRAW_SW_SUPPORT_I1           0

/*=================
 * OTHER SETTINGS
 *=================*/

#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1

#define LV_ATTRIBUTE_MEM_ALIGN __attribute__((aligned(4)))

/*==================
 *   FONT USAGE
 *===================*/

#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_30 1

#define LV_FONT_DEFAULT &lv_font_montserrat_22

/*==================
 * WIDGETS
 *================*/

#define LV_USE_ANIMIMG    0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   0
#define LV_USE_KEYBOARD   0
#define LV_USE_LED        0
#define LV_USE_SPINBOX    0
#define LV_USE_TABLE      0
#define LV_USE_TABVIEW    0

/*==================
 * EXAMPLES
 *==================*/

#define LV_BUILD_EXAMPLES 0

#endif /*LV_CONF_H*/

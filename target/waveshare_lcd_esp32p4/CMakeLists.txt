cmake_minimum_required (VERSION 3.21)

include_directories(../lvgl_setup)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project (maelir_waveshare_lcd_esp32p4 LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(.. maelir_target)
add_subdirectory(../display_jd9365 maelir_target_display)
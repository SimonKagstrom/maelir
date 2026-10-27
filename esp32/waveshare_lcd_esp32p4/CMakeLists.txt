cmake_minimum_required (VERSION 3.21)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project (maelir_waveshare_lcd_esp32p4 LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(.. maelir_target)
add_subdirectory(../../external/libmaelir/esp32/jd9365_esp32 maelir_target_display)
add_library(display_properties ALIAS display_800x800)

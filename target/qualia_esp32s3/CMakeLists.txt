cmake_minimum_required (VERSION 3.21)

include_directories(../lvgl_setup)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project (maelir_esp32s3_qualia LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(.. maelir_target)
add_subdirectory(../target_display maelir_target_display)

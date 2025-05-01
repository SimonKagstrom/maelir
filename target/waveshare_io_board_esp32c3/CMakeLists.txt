cmake_minimum_required (VERSION 3.21)

# Not actually needed here
include_directories(../lvgl_setup)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project (maelir_waveshare_io_board_esp32c3 LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

find_package(etl REQUIRED)

add_subdirectory(.. maelir)

cmake_minimum_required (VERSION 3.21)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project (maelir_esp32s3 LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

find_package(etl REQUIRED)
find_package(mp-units REQUIRED)

add_subdirectory(.. maelir)
add_subdirectory(target_display)
add_subdirectory(target_gps)
add_subdirectory(target_input)
add_subdirectory(os)

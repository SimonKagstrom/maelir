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
add_subdirectory(main/target_display)
add_subdirectory(main/target_gps)
add_subdirectory(main/target_input)
add_subdirectory(os)


#set(elf_file ${CMAKE_PROJECT_NAME}.elf)
#add_executable(${elf_file}
#    main.cc
#
#    os/base_thread_freertos.cc
#    os/semaphore_freertos.cc
#)
#
#target_link_libraries(${elf_file}
#    idf::freertos
#    idf::spi_flash
#    idf::esp_partition
#    encoder_input
#    tile_producer
#    gps_simulator
#    ui
#    gps_reader
#    target_display
#    uart_gps
#    mp-units::mp-units
#)

# Attach additional targets to the executable file for flashing,
# linker script generation, partition_table generation, etc.
#idf_build_executable(${elf_file})

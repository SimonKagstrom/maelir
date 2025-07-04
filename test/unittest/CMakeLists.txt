cmake_minimum_required (VERSION 3.21)
project (maelir_unittest LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

# Enable sanitizers in debug builds
add_compile_options(-fsanitize=address,undefined -g)
add_link_options(-fsanitize=address,undefined -g)
include_directories(../../qt/lvgl_setup)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

find_package(fmt REQUIRED)
find_package(etl REQUIRED)
find_package(trompeloeil REQUIRED)
find_package(doctest REQUIRED)

enable_testing()

add_subdirectory(../.. maelir)
add_subdirectory(../../external/libmaelir/test libmaelir_test)

add_executable(ut
    main.cc
    test_application_state.cc
    test_event_serializer.cc
    test_gps_reader.cc
    test_router.cc
    test_trip_computer.cc
)

target_link_libraries(ut
    application_state
    event_serializer
    nmea_parser
    route_iterator
    router
    timer_manager
    trip_computer
    gps_reader
    os_unittest
    doctest::doctest
    trompeloeil::trompeloeil
)
add_test(NAME unittest COMMAND ut)

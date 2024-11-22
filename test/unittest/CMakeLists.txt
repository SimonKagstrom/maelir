cmake_minimum_required (VERSION 3.21)
project (maelir_unittest LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Enable sanitizers in debug builds
add_compile_options(-fsanitize=address,undefined -g)
add_link_options(-fsanitize=address,undefined -g)

find_package(fmt REQUIRED)
find_package(etl REQUIRED)
find_package(trompeloeil REQUIRED)
find_package(doctest REQUIRED)

enable_testing()

add_subdirectory(../.. maelir)

add_executable(ut
    os/semaphore_unittest.cc
    main.cc
    test_application_state.cc
    test_nmea_parser.cc
    test_router.cc
    test_timer_manager.cc
)

target_link_libraries(ut
    application_state
    nmea_parser
    router
    router_interface
    timer_manager
    doctest::doctest
    trompeloeil::trompeloeil
)
add_test(NAME unittest COMMAND ut)

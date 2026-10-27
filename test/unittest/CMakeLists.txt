cmake_minimum_required (VERSION 3.21)
project (maelir_unittest LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

# Enable sanitizers in debug builds
add_compile_options(-fsanitize=address,undefined -g)
add_link_options(-fsanitize=address,undefined -g)

enable_testing()

set(TOP ${CMAKE_CURRENT_LIST_DIR}/../../)

add_subdirectory(${TOP} maelir)

add_library(generated_application_state ALIAS maelir_application_state)
add_library(messages ALIAS maelir_messages)

# After the aliases above, so that the libmaelir tests use the maelir state and messages (which
# include the common ones) instead of defining their own
add_subdirectory(${TOP}/external/libmaelir/test/unittest libmaelir_unittests)

# PNGdec does unaligned accesses on purpose
target_compile_options(pngdec_zlib PRIVATE -fno-sanitize=alignment)
target_compile_options(pngdec PRIVATE -fno-sanitize=alignment)

add_library(filesystem_implementation ALIAS mock_filesystem)
add_library(https_client_implementation ALIAS mock_https_client)
if(NOT TARGET display_properties)
    add_library(display_properties ALIAS display_480x480)
endif()


add_executable(unittest_maelir
    main.cc
    test_application_state.cc
    test_event_serializer.cc
    test_gps_reader.cc
    test_router.cc
    test_trip_computer.cc
)

target_link_libraries(unittest_maelir
    application_state
    event_serializer
    gps_reader
    os_unittest
    route_iterator
    router
    trip_computer

    doctest::doctest
    trompeloeil::trompeloeil
)
add_test(NAME unittest_maelir COMMAND unittest_maelir)

cmake_minimum_required(VERSION 3.21)
project(maelir_qt LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

# Qt setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Ignore some irritating warnings on MacOS
if(APPLE)
    add_link_options(-Wl,-no_warn_duplicate_libraries)

    # https://stackoverflow.com/questions/4929255/building-static-libraries-on-mac-using-cmake-and-gcc
    set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# Enable sanitizers in debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -g)
    add_link_options(-fsanitize=address,undefined -g)
else()
    add_compile_options(-g)
    add_link_options(-g)
endif()

add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(etl REQUIRED)
find_package(yaml-cpp REQUIRED)

add_subdirectory(.. maelir)
add_subdirectory(../external/libmaelir/qt libmaelir_qt)
add_subdirectory(uart_bridge)


add_executable(maelir_qt
    nvm_host.cc
    ota_updater_host.cc
    simulator_main.cc
    simulator_mainwindow.ui
    simulator_mainwindow.cc
)

target_link_libraries(maelir_qt
    display_qt
    os_qt
    ui
    tile_producer
    gps_simulator
    route_service
    storage
    trip_computer
    uart_bridge
    uart_event_listener
    uart_event_forwarder
    lvgl
    Qt6::Widgets
)

add_executable(map_editor
    mapeditor_graphicsview.cc
    mapeditor_main.cc
    mapeditor_mainwindow.ui
    mapeditor_mainwindow.cc
)
target_include_directories(map_editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(map_editor
    os_qt
    display_qt
    yaml-cpp
    route_iterator
    router
    router_interface
    Qt6::Widgets
)

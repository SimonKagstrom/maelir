cmake_minimum_required (VERSION 3.21)
project (maelir_qt LANGUAGES CXX C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

# Qt setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


# Ignore some irritating warnings on MacOS
if (APPLE)
    add_link_options(-Wl,-no_warn_duplicate_libraries)
    # https://stackoverflow.com/questions/4929255/building-static-libraries-on-mac-using-cmake-and-gcc
    set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()

# Enable sanitizers in debug builds
add_compile_options(-fsanitize=address,undefined -g)
add_link_options(-fsanitize=address,undefined -g)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(fmt REQUIRED)
find_package(etl REQUIRED)

add_subdirectory(.. maelir)

add_library(os_qt EXCLUDE_FROM_ALL
    os/base_thread_qt.cc
    os/semaphore_qt.cc
)

target_link_libraries(os_qt
    Qt6::Widgets
    fmt::fmt
    maelir_interface
)

add_library(mainwindow EXCLUDE_FROM_ALL
    mainwindow.ui
    mainwindow.cc
    display_qt.cc
)

target_link_libraries(mainwindow
PUBLIC
    Qt6::Widgets
    fmt::fmt
    maelir_interface
    os_qt

# TMP!
    tile_producer
    ui
    )

add_executable(maelir_qt
    gps_simulator.cc
    main.cc
)

target_link_libraries(maelir_qt
    mainwindow
    ui
    tile_producer
)

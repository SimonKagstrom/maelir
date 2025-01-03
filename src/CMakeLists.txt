set(RESOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../resources CACHE PATH "Path to resources directory")

add_subdirectory(application_state)
add_subdirectory(gps_reader)
add_subdirectory(gps_simulator)
add_subdirectory(nmea_parser)
add_subdirectory(route_service)
add_subdirectory(router)
add_subdirectory(tile_producer)
add_subdirectory(timer_manager)
add_subdirectory(ui)


add_library(maelir_interface INTERFACE)

target_include_directories(maelir_interface INTERFACE
    include
)

target_link_libraries(maelir_interface INTERFACE
    etl::etl
    fmt::fmt
    lvgl
)

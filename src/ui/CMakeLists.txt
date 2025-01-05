library_from_data(${RESOURCE_DIR}/boat.png boat)

add_library(ui EXCLUDE_FROM_ALL
    map_screen.cc
    menu_screen.cc
    ui.cc
)

target_include_directories(ui
PUBLIC
    include
)

target_link_libraries(ui
PUBLIC
    application_state
    maelir_interface
    tile_producer
    gps_reader
    route_service
    lvgl
PRIVATE
    boat_library
    painter
)

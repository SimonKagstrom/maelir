library_from_data(${RESOURCE_DIR}/boat.png boat)
library_from_data(${RESOURCE_DIR}/crosshair.png crosshair)

add_library(ui EXCLUDE_FROM_ALL
    map_screen.cc
    menu_screen.cc
    ui.cc
    updating_screen.cc
)

target_include_directories(ui
PUBLIC
    include
)

target_link_libraries(ui
PUBLIC
    application_state
    base_thread
    tile_producer
    gps_reader
    route_service
    lvgl
    ota_updater
PRIVATE
    boat_library
    crosshair_library
    painter
    route_iterator
)

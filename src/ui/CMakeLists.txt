library_from_data(${RESOURCE_DIR}/boat.png boat)
library_from_data(${RESOURCE_DIR}/numbers.png numbers)

add_library(ui EXCLUDE_FROM_ALL
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
    numbers_library
)

add_library(ui EXCLUDE_FROM_ALL
    ui.cc
)

target_include_directories(ui
PUBLIC
    include
)

target_link_libraries(ui
PUBLIC
    maelir_interface
    tile_producer
    gps_reader
    route_service
PRIVATE
    painter
)

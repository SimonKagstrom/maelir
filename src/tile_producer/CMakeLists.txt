add_library(tile_producer EXCLUDE_FROM_ALL
    tile_producer.cc
)

target_include_directories(tile_producer
PUBLIC
    include
)

target_link_libraries(tile_producer
PUBLIC
    painter
    gps_reader
    application_state
PRIVATE
    pngdec
)

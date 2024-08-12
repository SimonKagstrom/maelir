tilegen(tile_library "/Users/ska/Downloads/sjokort.png")

add_library(tile_producer EXCLUDE_FROM_ALL
    tile_producer.cc
)

target_include_directories(tile_producer
PUBLIC
    include
)

target_link_libraries(tile_producer
PUBLIC
    maelir_interface
    etl::etl
    gps_reader
    tile_library
PRIVATE
    PNG::PNG
    fmt::fmt
)

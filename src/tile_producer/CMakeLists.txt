if ("${MAP_IMAGE_PATH}" STREQUAL "")
    message(FATAL_ERROR "-DMAP_IMAGE_PATH=<path_to_png_image> is required when running cmake")
endif()


tilegen(tile_library ${MAP_IMAGE_PATH})

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
    fmt::fmt
    pngdec
)

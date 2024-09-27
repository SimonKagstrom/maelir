include(../../cmake/tilegen.cmake)

if ("${MAP_YAML_PATH}" STREQUAL "")
    message(FATAL_ERROR "-DMAP_YAML_PATH=<path_to_maelir_yaml_metadata> is required when running cmake")
endif()

tilegen(${MAP_YAML_PATH})

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
    gps_reader
PRIVATE
    pngdec
)
add_dependencies(tile_producer generated_map_data)

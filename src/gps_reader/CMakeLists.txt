add_library(tile_position_converter EXCLUDE_FROM_ALL
    tile_position_converter.cc
)

target_include_directories(tile_position_converter
PUBLIC
    include
)

target_link_libraries(tile_position_converter
PUBLIC
    maelir_interface
)



add_library(gps_reader EXCLUDE_FROM_ALL
    gps_reader.cc
)

target_include_directories(gps_reader
PUBLIC
    include
)

target_link_libraries(gps_reader
PUBLIC
    base_thread
    # For now, this could be an interface with several converters in the future.
    tile_position_converter
    application_state
)

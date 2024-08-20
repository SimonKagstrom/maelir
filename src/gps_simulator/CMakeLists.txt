add_library(gps_simulator EXCLUDE_FROM_ALL
    gps_simulator.cc
)

target_include_directories(gps_simulator
PUBLIC
    include
)

target_link_libraries(gps_simulator
PUBLIC
    maelir_interface
PRIVATE
    # tile_utils, tiles
    tile_producer
)

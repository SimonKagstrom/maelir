add_subdirectory(gps_reader)
add_subdirectory(tile_producer)
add_subdirectory(ui)


add_library(maelir_interface INTERFACE)

target_include_directories(maelir_interface INTERFACE
    include
)

target_link_libraries(maelir_interface INTERFACE
    etl::etl
)

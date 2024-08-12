add_library(gps_reader EXCLUDE_FROM_ALL
    gps_reader.cc
)

target_include_directories(gps_reader
PUBLIC
    include
)

target_link_libraries(gps_reader
PUBLIC
    maelir_interface
)

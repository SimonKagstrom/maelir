add_library(nmea_parser EXCLUDE_FROM_ALL
    nmea_parser.cc
)

target_link_libraries(nmea_parser
PUBLIC
    maelir_interface
)

target_include_directories(nmea_parser
PUBLIC
    include
)

add_library(nmea_parser EXCLUDE_FROM_ALL
    nmea_parser.cc
)

target_link_libraries(nmea_parser
PUBLIC
    maelir_interface
)

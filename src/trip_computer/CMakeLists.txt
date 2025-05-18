add_library(trip_computer EXCLUDE_FROM_ALL
    trip_computer.cc
)

target_include_directories(trip_computer
PUBLIC
    include
)

target_link_libraries(trip_computer
PUBLIC
    route_service
    base_thread
    gps_reader
)

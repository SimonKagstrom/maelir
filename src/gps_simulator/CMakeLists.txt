add_library(gps_simulator EXCLUDE_FROM_ALL
    gps_simulator.cc
)

target_include_directories(gps_simulator
PUBLIC
    include
)

target_link_libraries(gps_simulator
PUBLIC
    base_thread
    route_service
    application_state
PRIVATE
    gps_reader
)

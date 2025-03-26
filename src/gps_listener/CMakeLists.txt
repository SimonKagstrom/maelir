add_library(gps_listener EXCLUDE_FROM_ALL
    gps_listener.cc
)

target_include_directories(gps_listener
PUBLIC
    include
)

target_link_libraries(gps_listener
PUBLIC
    base_thread
)

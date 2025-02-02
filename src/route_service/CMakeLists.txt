add_library(route_service EXCLUDE_FROM_ALL
    route_service.cc
)

target_include_directories(route_service
PUBLIC
    include
)

target_link_libraries(route_service
PUBLIC
    router
    base_thread
)

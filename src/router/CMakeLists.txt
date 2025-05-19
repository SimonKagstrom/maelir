add_library(router_interface INTERFACE)

target_include_directories(router_interface INTERFACE
    include)

target_link_libraries(router_interface INTERFACE
    maelir_interface
    tile_position_converter
)


add_library(route_iterator EXCLUDE_FROM_ALL
    route_iterator.cc
)

target_link_libraries(route_iterator
PUBLIC
    router_interface
)


add_library(router EXCLUDE_FROM_ALL
    router.cc
)

target_link_libraries(router
PUBLIC
    router_interface
)

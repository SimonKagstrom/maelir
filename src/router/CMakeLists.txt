add_library(router_interface INTERFACE)
target_include_directories(router_interface INTERFACE
    include)

add_library(router EXCLUDE_FROM_ALL
    route_iterator.cc
    router.cc
)
target_link_libraries(router
PUBLIC
    maelir_interface
    router_interface
)

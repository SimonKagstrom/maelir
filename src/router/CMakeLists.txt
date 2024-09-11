add_library(router_internal_interface INTERFACE)
target_include_directories(router_internal_interface INTERFACE
    ${CMAKE_CURRENT_LIST_DIR})

add_library(router EXCLUDE_FROM_ALL
    route_iterator.cc
    router.cc
)

target_include_directories(router
PUBLIC
    include
)

target_link_libraries(router
PUBLIC
    maelir_interface
PRIVATE
    router_internal_interface
)

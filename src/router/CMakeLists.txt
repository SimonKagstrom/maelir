add_library(router EXCLUDE_FROM_ALL
    router.cc
)

target_include_directories(router
PUBLIC
    include
)

target_link_libraries(router
PUBLIC
    maelir_interface
)

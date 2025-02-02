add_library(timer_manager EXCLUDE_FROM_ALL
    timer_manager.cc
)

target_link_libraries(timer_manager
PUBLIC
    maelir_interface
)

target_include_directories(timer_manager
PUBLIC
    include
)

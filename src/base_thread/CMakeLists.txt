add_library(base_thread INTERFACE)

target_include_directories(base_thread INTERFACE
    include
)

target_link_libraries(base_thread INTERFACE
    maelir_interface
    timer_manager
)

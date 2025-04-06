add_library(button_debouncer EXCLUDE_FROM_ALL
    button_debouncer.cc
)

target_include_directories(button_debouncer
PUBLIC
    include
)

target_link_libraries(button_debouncer
PUBLIC
    maelir_interface
    base_thread
)

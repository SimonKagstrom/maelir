add_library(uart_bridge EXCLUDE_FROM_ALL
    uart_bridge.cc
)

target_include_directories(uart_bridge
PUBLIC
    include
)

target_link_libraries(uart_bridge
PUBLIC
    maelir_interface
)

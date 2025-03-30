add_library(uart_event_listener EXCLUDE_FROM_ALL
    uart_event_listener.cc
)

target_include_directories(uart_event_listener
PUBLIC
    include
)

target_link_libraries(uart_event_listener
PUBLIC
    base_thread
    event_serializer
PRIVATE
)

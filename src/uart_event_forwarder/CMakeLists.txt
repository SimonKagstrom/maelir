add_library(uart_event_forwarder EXCLUDE_FROM_ALL
    uart_event_forwarder.cc
)

target_include_directories(uart_event_forwarder
PUBLIC
    include
)

target_link_libraries(uart_event_forwarder
PUBLIC
    base_thread
    gps_listener
PRIVATE
    event_serializer
)

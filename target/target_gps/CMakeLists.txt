add_library(uart_gps EXCLUDE_FROM_ALL
    uart_gps.cc
)

target_include_directories(uart_gps
PUBLIC
    include
)

target_link_libraries(uart_gps
PUBLIC
    idf::freertos
    idf::esp_driver_uart
    idf::esp_timer
    maelir_interface
)

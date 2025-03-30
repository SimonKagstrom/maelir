add_library(target_uart EXCLUDE_FROM_ALL
    target_uart.cc
)

target_include_directories(target_uart
PUBLIC
    include
)

target_link_libraries(target_uart
PUBLIC
    idf::esp_driver_uart
    maelir_interface
)

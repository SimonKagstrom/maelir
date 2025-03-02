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
    nmea_parser
)


add_library(i2c_gps EXCLUDE_FROM_ALL
    i2c_gps.cc
)

target_include_directories(i2c_gps
PUBLIC
    include
)

target_link_libraries(i2c_gps
PUBLIC
    idf::freertos
    idf::esp_driver_i2c
    maelir_interface
    nmea_parser
)

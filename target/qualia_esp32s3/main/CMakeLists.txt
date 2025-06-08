# Register the component
idf_component_register(SRCS "main.cc" INCLUDE_DIRS "")

# ... but use regular CMake for the rest
target_link_libraries(${COMPONENT_TARGET}
    idf::freertos
    idf::spi_flash
    idf::esp_partition
    encoder_input
    button_debouncer
    tile_producer
    gps_simulator
    ui
    gps_reader
    ota_updater
    target_gpio
    target_uart
    target_display
    target_os
    target_nvm
    trip_computer
    target_httpd_ota_updater
    storage
    uart_gps
    i2c_gps
)

# Register the component
idf_component_register(SRCS "main.cc" INCLUDE_DIRS "")

# ... but use regular CMake for the rest
target_link_libraries(${COMPONENT_TARGET}
    idf::freertos
    idf::spi_flash
    idf::esp_partition
    encoder_input
    tile_producer
    trip_computer
    gps_simulator
    ui
    gps_reader
    ota_updater
    target_httpd_ota_updater
    target_os
    target_nvm
    storage
    uart_gps
    i2c_gps
    display_jd9365
    uart_event_listener
)

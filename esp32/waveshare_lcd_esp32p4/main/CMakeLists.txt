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
    httpd_ota_updater_esp32
    os_esp32
    nvm_esp32
    uart_esp32
    storage
    uart_gps_esp32
    i2c_gps_esp32
    jd9365_display_esp32
    uart_event_listener
)

# Register the component
idf_component_register(SRCS "main.cc" INCLUDE_DIRS "")

# ... but use regular CMake for the rest
target_link_libraries(${COMPONENT_TARGET}
    idf::freertos
    idf::spi_flash
    idf::esp_partition
    button_debouncer
    encoder_input
    gpio_esp32
    gps_reader
    gps_simulator
    httpd_ota_updater_esp32
    i2c_gps_esp32
    nvm_esp32
    os_esp32
    ota_updater
    st7701_display_esp32
    storage
    tile_producer
    trip_computer
    uart_esp32
    uart_gps_esp32
    ui
)

# Register the component
idf_component_register(SRCS "main.cc" INCLUDE_DIRS "")

# ... but use regular CMake for the rest
target_link_libraries(${COMPONENT_TARGET}
    idf::freertos
    idf::spi_flash
    idf::esp_partition
    encoder_input
    tile_producer
    gps_simulator
    ui
    gps_reader
    target_display
    target_os
    target_nvm
    storage
    uart_gps
    i2c_gps
    uart_event_listener
)

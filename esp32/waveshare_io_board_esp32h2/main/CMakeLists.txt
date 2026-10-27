# Register the component
idf_component_register(SRCS "main.cc" INCLUDE_DIRS "")

# ... but use regular CMake for the rest
target_link_libraries(${COMPONENT_TARGET}
    idf::freertos
    idf::spi_flash
    idf::esp_partition
    encoder_input
    uart_gps_esp32
    os_esp32
    nvm_esp32
    gps_listener
    button_debouncer
    gpio_esp32
    uart_esp32
    event_serializer
    uart_event_forwarder
)

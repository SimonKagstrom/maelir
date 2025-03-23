# Register the component
idf_component_register(SRCS "main.cc" INCLUDE_DIRS "")

# ... but use regular CMake for the rest
target_link_libraries(${COMPONENT_TARGET}
    idf::freertos
    idf::spi_flash
    idf::esp_partition
    encoder_input
    uart_gps
    target_os
    target_nvm
)

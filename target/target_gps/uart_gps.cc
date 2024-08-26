#include "uart_gps.hh"

#include <driver/uart.h>

constexpr auto kUartBufSize = 256;

UartGps::UartGps(uart_port_t port_number, uint8_t rx_pin, uint8_t tx_pin)
    : m_port_number(port_number)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    auto intr_alloc_flags = 0;

    ESP_ERROR_CHECK(
        uart_driver_install(m_port_number, kUartBufSize * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(m_port_number, &uart_config));
    ESP_ERROR_CHECK(
        uart_set_pin(m_port_number, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

GpsData
UartGps::WaitForData(os::binary_semaphore& semaphore)
{
    char data[192];
    auto len = uart_read_bytes(m_port_number, data, sizeof(data), portMAX_DELAY);

    if (len)
    {
        data[sizeof(data) - 1] = '\0';
        printf("Received %d bytes: %s\n", len, data);
    }
    semaphore.release();

    return {0, 0, 0, 0};
}
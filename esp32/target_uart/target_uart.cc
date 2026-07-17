#include "target_uart.hh"

constexpr auto kUartBufSize = 128;

TargetUart::TargetUart(uart_port_t port_number, int baudrate, uint8_t rx_pin, uint8_t tx_pin)
    : m_port_number(port_number)
{
    uart_config_t uart_config = {
        .baud_rate = baudrate,
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

void
TargetUart::Write(std::span<const uint8_t> data)
{
    uart_write_bytes(m_port_number, reinterpret_cast<const char*>(data.data()), data.size());
}

std::span<uint8_t>
TargetUart::Read(std::span<uint8_t> data, milliseconds timeout)
{
    size_t in_hw_buffer = 0;
    uart_get_buffered_data_len(m_port_number, &in_hw_buffer);

    auto wanted = std::max(1u, in_hw_buffer);
    auto len = uart_read_bytes(m_port_number,
                               reinterpret_cast<char*>(data.data()),
                               std::min(wanted, data.size()),
                               pdMS_TO_TICKS(timeout.count()));
    if (len > 0)
    {
        return data.subspan(0, len);
    }
    else
    {
        return {};
    }
}

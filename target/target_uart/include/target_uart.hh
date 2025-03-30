#pragma once

#include "hal/i_uart.hh"
#include <driver/uart.h>

class TargetUart : public hal::IUart
{
public:
    TargetUart(uart_port_t port_number, int baudrate, uint8_t rx_pin, uint8_t tx_pin);

private:
    void Write(std::span<const uint8_t> data) final;
    std::span<uint8_t> Read(std::span<uint8_t> data, milliseconds timeout) final;

    const uart_port_t m_port_number;
};

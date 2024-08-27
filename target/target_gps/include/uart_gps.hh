#pragma once

#include "hal/i_gps.hh"

#include <array>
#include <etl/vector.h>
#include <driver/uart.h>

class UartGps : public hal::IGps
{
public:
    UartGps(uart_port_t port_number, uint8_t rx_pin, uint8_t tx_pin);

private:
    GpsData WaitForData(os::binary_semaphore& semaphore) final;

    void OnLine(std::string_view line);

    const uart_port_t m_port_number;
    etl::vector<char, 256> m_line;
};

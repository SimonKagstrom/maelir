#pragma once

#include "hal/i_gps.hh"
#include "nmea_parser.hh"

#include <array>
#include <driver/uart.h>
#include <etl/vector.h>

class UartGps : public hal::IGps
{
public:
    UartGps(uart_port_t port_number, uint8_t rx_pin, uint8_t tx_pin);

private:
    std::optional<hal::RawGpsData> WaitForData(os::binary_semaphore& semaphore) final;

    void OnLine(std::string_view line);

    const uart_port_t m_port_number;
    etl::vector<char, 256> m_line;

    std::unique_ptr<NmeaParser> m_parser;
};

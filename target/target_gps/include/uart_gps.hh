#pragma once

#include "hal/i_gps.hh"
#include "hal/i_uart.hh"
#include "nmea_parser.hh"

#include <array>
#include <driver/uart.h>

class UartGps : public hal::IGps
{
public:
    UartGps(hal::IUart &uart);

private:
    std::optional<hal::RawGpsData> WaitForData(os::binary_semaphore& semaphore) final;

    void OnLine(std::string_view line);

    hal::IUart &m_uart;
    std::array<uint8_t, 256> m_buffer;

    std::unique_ptr<NmeaParser> m_parser;
};

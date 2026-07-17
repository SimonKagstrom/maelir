#include "uart_gps.hh"

#include <driver/uart.h>

constexpr auto kUartBufSize = 128;

UartGps::UartGps(hal::IUart& uart)
    : m_uart(uart)
    , m_parser(std::make_unique<NmeaParser>())
{
}

std::optional<hal::RawGpsData>
UartGps::WaitForData(os::binary_semaphore& semaphore)
{
    std::optional<hal::RawGpsData> data;

    auto s = m_uart.Read(m_buffer, 1s);

    if (s.size() > 0)
    {
        data = m_parser->PushData(std::string_view((const char*)s.data(), s.size()));
    }
    semaphore.release();

    return data;
}
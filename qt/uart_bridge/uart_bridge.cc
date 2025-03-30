#include "uart_bridge.hh"

#include <cassert>

UartBridge::UartBridge()
    : m_uart_a(m_input_buffer, m_input_semaphore)
    , m_uart_b(m_input_buffer, m_input_semaphore)
{
    m_uart_a.AttachPeer(&m_uart_b);
    m_uart_b.AttachPeer(&m_uart_a);
}

std::pair<hal::IUart&, hal::IUart&>
UartBridge::GetEndpoints()
{
    return {m_uart_a, m_uart_b};
}

UartBridge::BridgedUart::BridgedUart(etl::queue_spsc_atomic<uint8_t, 1024>& input_buffer,
                                     os::binary_semaphore& input_semaphore)
    : m_input_buffer(input_buffer)
    , m_input_semaphore(input_semaphore)
{
}

void
UartBridge::BridgedUart::AttachPeer(hal::IUart* peer)
{
    m_peer = peer;
}

void
UartBridge::BridgedUart::Write(std::span<const uint8_t> data)
{
    assert(m_peer);
    for (auto b : data)
    {
        m_input_buffer.push(b);
    }
    m_input_semaphore.release();
}

std::span<uint8_t>
UartBridge::BridgedUart::Read(std::span<uint8_t> data, milliseconds timeout)
{
    if (m_input_buffer.empty())
    {
        m_input_semaphore.try_acquire_for(timeout);
    }

    uint8_t b;
    int i = 0;
    while (m_input_buffer.pop(b))
    {
        if (i >= data.size())
        {
            break;
        }
        data[i] = b;
        i++;
    }

    return data.subspan(0, i);
}

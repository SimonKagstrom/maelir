#pragma once

#include "hal/i_uart.hh"
#include "semaphore.hh"

#include <etl/queue_spsc_atomic.h>
#include <utility>
#include <vector>


class UartBridge
{
public:
    UartBridge();

    std::pair<hal::IUart&, hal::IUart&> GetEndpoints();

private:
    class BridgedUart : public hal::IUart
    {
    public:
        friend class UartBridge;

        void AttachPeer(hal::IUart* peer);

        BridgedUart(const BridgedUart&) = delete;
        BridgedUart& operator=(const BridgedUart&) = delete;

    private:
        BridgedUart(        etl::queue_spsc_atomic<uint8_t, 1024> &input_buffer, os::binary_semaphore& input_semaphore);

        void Write(std::span<const uint8_t> data) final;
        std::span<uint8_t> Read(std::span<uint8_t> data, milliseconds timeout) final;

        hal::IUart* m_peer {nullptr};

        etl::queue_spsc_atomic<uint8_t, 1024> &m_input_buffer;
        os::binary_semaphore& m_input_semaphore;
    };

    BridgedUart m_uart_a;
    BridgedUart m_uart_b;

    etl::queue_spsc_atomic<uint8_t, 1024> m_input_buffer;
    os::binary_semaphore m_input_semaphore {0};
};

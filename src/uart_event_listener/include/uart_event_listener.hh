#pragma once

#include "base_thread.hh"
#include "event_serializer.hh"
#include "hal/i_gps.hh"
#include "hal/i_input.hh"
#include "hal/i_uart.hh"

#include <array>
#include <etl/queue_spsc_atomic.h>

class UartEventListener : public hal::IInput, public hal::IGps, public os::BaseThread
{
public:
    UartEventListener(hal::IUart& uart);

private:
    // os::BaseThread
    std::optional<milliseconds> OnActivation() final;

    // IInput
    void AttachListener(hal::IInput::IListener* listener) final;
    hal::IInput::State GetState() final;

    // IGps Context: Another thread
    std::optional<hal::RawGpsData> WaitForData(os::binary_semaphore& semaphore) final;

    hal::IUart& m_uart;
    hal::IInput::State m_state {0};
    hal::IInput::IListener* m_listener {nullptr};
    etl::queue_spsc_atomic<hal::RawGpsData, 16> m_gps_queue;

    std::array<uint8_t, 32> m_buffer;
    serializer::Deserializer m_deserializer;

    os::binary_semaphore m_gps_semaphore {0};
};

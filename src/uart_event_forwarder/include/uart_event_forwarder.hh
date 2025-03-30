#pragma once

#include "gps_listener.hh"
#include "hal/i_input.hh"
#include "hal/i_uart.hh"

#include <etl/queue_spsc_atomic.h>

class UartEventForwarder : public os::BaseThread, public hal::IInput::IListener
{
public:
    UartEventForwarder(hal::IUart& send_uart, hal::IInput& input, GpsListener& gps_listener);

private:
    std::optional<milliseconds> OnActivation() final;
    void OnInput(const hal::IInput::Event& event) final;

    hal::IUart& m_send_uart;
    hal::IInput& m_input;
    etl::queue_spsc_atomic<hal::IInput::Event, 16> m_input_queue;
    etl::queue_spsc_atomic<hal::RawGpsData, 16> m_gps_queue;

    etl::vector<uint8_t, 256> m_buffer;
};

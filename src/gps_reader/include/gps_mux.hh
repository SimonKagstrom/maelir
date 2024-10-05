#pragma once

#include "application_state.hh"
#include "hal/i_gps.hh"

class GpsMux : public hal::IGps
{
public:
    GpsMux(ApplicationState& application_state, hal::IGps& uart, hal::IGps& simulator)
        : m_application_state(application_state)
        , m_uart(uart)
        , m_simulator(simulator)
    {
    }

    hal::RawGpsData WaitForData(os::binary_semaphore& semaphore) final
    {
        if (m_application_state.CheckoutReadonly()->demo_mode)
        {
            return m_simulator.WaitForData(semaphore);
        }

        return m_uart.WaitForData(semaphore);
    }

private:
    ApplicationState& m_application_state;
    hal::IGps& m_uart;
    hal::IGps& m_simulator;
};
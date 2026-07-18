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

    std::optional<hal::RawGpsData> WaitForData(IEventNotifier& notifier) final
    {
        if (m_application_state.CheckoutReadonly().Get<AS::demo_mode>())
        {
            return m_simulator.WaitForData(notifier);
        }

        return m_uart.WaitForData(notifier);
    }

private:
    ApplicationState& m_application_state;
    hal::IGps& m_uart;
    hal::IGps& m_simulator;
};
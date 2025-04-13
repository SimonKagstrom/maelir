#pragma once

#include "base_thread.hh"
#include "hal/i_gpio.hh"

#include <atomic>

class ButtonDebouncer : public os::BaseThread, public hal::IGpio
{
public:
    ButtonDebouncer(hal::IGpio& button_gpio);

private:
    // from hal::IGpio
    bool GetState() const final;

    void SetState(bool state) final
    {
        // Not relevant
    }

    std::unique_ptr<ListenerCookie>
    AttachIrqListener(std::function<void(bool)> on_state_change) final;

    // From os::BaseThread
    std::optional<milliseconds> OnActivation() final;

    hal::IGpio& m_button_gpio;
    std::function<void(bool)> m_on_state_change {[](auto) {}};

    std::unique_ptr<ListenerCookie> m_interrupt_listener;

    uint8_t m_button_history {0};
    uint8_t m_history_count {0};
    std::atomic_bool m_state {false};

    os::TimerHandle m_timer {nullptr};

    std::atomic_bool m_interrupt {false};
};

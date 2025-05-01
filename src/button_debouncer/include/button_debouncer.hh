#pragma once

#include "base_thread.hh"
#include "hal/i_gpio.hh"

#include <atomic>
#include <etl/vector.h>

class ButtonDebouncer : public os::BaseThread
{
public:
    std::unique_ptr<hal::IGpio> AddButton(std::unique_ptr<hal::IGpio> pin);

private:
    class Button final : public hal::IGpio
    {
    public:
        Button(ButtonDebouncer& parent, std::unique_ptr<hal::IGpio> pin);

        ~Button() override;

        void ScanButton();

    private:
        // from hal::IGpio
        bool GetState() const override;

        void SetState(bool state) override
        {
            // Not relevant
        }

        std::unique_ptr<ListenerCookie>
        AttachIrqListener(std::function<void(bool)> on_state_change) final;

        ButtonDebouncer& m_parent;
        std::unique_ptr<hal::IGpio> m_button_gpio;
        std::function<void(bool)> m_on_state_change {[](auto) {}};

        std::unique_ptr<ListenerCookie> m_interrupt_listener;

        uint8_t m_button_history {0};
        uint8_t m_history_count {0};
        std::atomic_bool m_state {false};

        os::TimerHandle m_timer {nullptr};

        std::atomic_bool m_interrupt {false};
    };

    void RemoveButton(Button* button);

    // From os::BaseThread
    std::optional<milliseconds> OnActivation() final;

    etl::vector<Button*, 4> m_buttons;
};

#pragma once

#include "hal/i_gpio.hh"

#include <driver/gpio.h>

class TargetGpio : public hal::IGpio
{
public:
    enum class Polarity
    {
        kActiveHigh,
        kActiveLow,

        kValueCount,
    };

    // TODO: Handle output
    TargetGpio(uint8_t pin, Polarity polarity = Polarity::kActiveHigh);

    bool GetState() const final;

    void SetState(bool state) final;

    std::unique_ptr<ListenerCookie>
    AttachIrqListener(std::function<void(bool)> on_state_change) final;

private:
    void ButtonIsr();
    static void StaticButtonIsr(void* arg);

    const gpio_num_t m_pin;
    const uint8_t m_polarity;
    std::function<void(bool)> m_on_state_change {[](auto) {}};
};
#pragma once

#include "hal/i_input.hh"
#include "rotary_encoder.hh"
#include "time.hh"

#include <driver/gpio.h>

class EncoderInput : public hal::IInput
{
public:
    EncoderInput(RotaryEncoder& rotary_encoder, hal::IGpio& button, uint8_t switch_up_pin);

private:
    void AttachListener(hal::IInput::IListener* listener) final;
    State GetState() final;

    void ButtonIsr();
    static void StaticButtonIsr(void* arg);


    hal::IGpio& m_button;
    const gpio_num_t m_pin_switch_up;
    hal::IInput::IListener* m_listener {nullptr};

    std::unique_ptr<ListenerCookie> m_button_listener;
    std::unique_ptr<ListenerCookie> m_rotary_listener;
};

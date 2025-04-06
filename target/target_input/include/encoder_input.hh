#pragma once

#include "button_debouncer.hh"
#include "hal/i_input.hh"
#include "time.hh"

#include <driver/gpio.h>
#include <driver/pulse_cnt.h>

class EncoderInput : public hal::IInput
{
public:
    EncoderInput(uint8_t pin_a, uint8_t pin_b, ButtonDebouncer& button, uint8_t switch_up_pin);

private:
    void AttachListener(hal::IInput::IListener* listener) final;
    State GetState() final;

    void PcntOnReach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t* edata);
    static bool StaticPcntOnReach(pcnt_unit_handle_t unit,
                                  const pcnt_watch_event_data_t* edata,
                                  void* user_ctx);

    void ButtonIsr();
    static void StaticButtonIsr(void* arg);


    ButtonDebouncer& m_button;
    const gpio_num_t m_pin_switch_up;
    hal::IInput::IListener* m_listener {nullptr};
    pcnt_unit_handle_t m_pcnt_unit;

    std::unique_ptr<ListenerCookie> m_button_listener;
};

#pragma once

#include "hal/i_input.hh"

#include <driver/pulse_cnt.h>

class EncoderInput : public hal::IInput
{
public:
    EncoderInput(uint8_t pin_a, uint8_t pin_b, uint8_t pin_button);

private:
    void AttachListener(hal::IInput::IListener* listener) final;

    void PcntOnReach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t* edata);
    static bool StaticPcntOnReach(pcnt_unit_handle_t unit,
                                  const pcnt_watch_event_data_t* edata,
                                  void* user_ctx);


    hal::IInput::IListener* m_listener;
    pcnt_unit_handle_t m_pcnt_unit;
};

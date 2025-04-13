#pragma once

#include "hal/i_gpio.hh"
#include "listener_cookie.hh"

#include <etl/vector.h>

class RotaryEncoder
{
public:
    enum class Direction
    {
        kLeft,
        kRight,

        kValueCount,
    };

    RotaryEncoder(hal::IGpio& pin_a, hal::IGpio& pin_b);

    std::unique_ptr<ListenerCookie> AttachIrqListener(std::function<void(Direction)> on_rotation);

private:
    void Process();

    std::function<void(Direction)> m_on_rotation {[](auto) {}};

    hal::IGpio& m_pin_a;
    hal::IGpio& m_pin_b;

    etl::vector<std::unique_ptr<ListenerCookie>, 2> m_listeners;

    uint8_t m_state {0};
};

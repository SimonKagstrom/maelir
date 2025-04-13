#pragma once

#include "button_debouncer.hh"
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

    RotaryEncoder(ButtonDebouncer& pin_a, ButtonDebouncer& pin_b);

    std::unique_ptr<ListenerCookie> AttachListener(std::function<void(Direction)> on_rotation);

private:
    void Process();

    std::function<void(Direction)> m_on_rotation {[](auto) {}};

    ButtonDebouncer& m_pin_a;
    ButtonDebouncer& m_pin_b;

    etl::vector<std::unique_ptr<ListenerCookie>, 2> m_listeners;

    uint8_t m_state {0};
};

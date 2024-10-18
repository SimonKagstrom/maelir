#pragma once

#include <cstdint>

namespace hal
{

class IInput
{
public:
    enum class EventType
    {
        kButtonDown,
        kButtonUp,
        kLeft,
        kRight,

        kValueCount,
    };

    struct Event
    {
        uint8_t button;
        EventType type;
    };

    class IListener
    {
    public:
        virtual ~IListener() = default;

        // Context: IRQ
        virtual void OnInput(const Event& event) = 0;
    };

    virtual ~IInput() = default;

    virtual void AttachListener(IListener* listener) = 0;
};

}
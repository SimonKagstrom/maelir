#pragma once

#include <cstdint>
#include <utility>

namespace hal
{

class IInput
{
public:
    enum class EventType: uint8_t
    {
        kButtonDown,
        kButtonUp,
        kLeft,
        kRight,

        kSwitchUp,
        kSwitchNeutral,
        kSwitchDown,

        kValueCount,
    };

    enum class StateType : uint8_t
    {
        kButtonDown = 1,

        kSwitchUp = 2,
        // Neutral is when neither of these are set
        kSwitchDown = 4,
    };

    class State
    {
    public:
        bool IsActive(StateType type) const
        {
            return m_state & std::to_underlying(type);
        }

        // Consider this as private to the input driver
        State(uint8_t state)
            : m_state(state)
        {
        }

    private:
        const uint8_t m_state;
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

    /// Return the current state (buttons down)
    virtual State GetState() = 0;
};

} // namespace hal

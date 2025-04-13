// Based on https://github.com/brianlow/Rotary
#include "rotary_encoder.hh"

// Values returned by 'process'
// No complete step yet.
constexpr uint8_t kDirNone = 0x00;
// Clockwise step.
constexpr uint8_t kDirCw = 0x10;
// Counter-clockwise step.
constexpr uint8_t kDirCCw = 0x20;

constexpr uint8_t kStart = 0x00;

constexpr uint8_t kCwFinal = 0x1;
constexpr uint8_t kCwBegin = 0x2;
constexpr uint8_t kCwNext = 0x3;
constexpr uint8_t kCcwBegin = 0x4;
constexpr uint8_t kCcwFinal = 0x5;
constexpr uint8_t kCcwNext = 0x6;

constexpr uint8_t kTable[7][4] = {
    // kStart
    {kStart, kCwBegin, kCcwBegin, kStart},
    // kCwFinal
    {kCwNext, kStart, kCwFinal, kStart | kDirCw},
    // kCwBegin
    {kCwNext, kCwBegin, kStart, kStart},
    // kCwNext
    {kCwNext, kCwBegin, kCwFinal, kStart},
    // kCcwBegin
    {kCcwNext, kStart, kCcwBegin, kStart},
    // kCcwFinal
    {kCcwNext, kCcwFinal, kStart, kStart | kDirCCw},
    // kCcwNext
    {kCcwNext, kCcwFinal, kCcwBegin, kStart},
};

RotaryEncoder::RotaryEncoder(ButtonDebouncer& pin_a, ButtonDebouncer& pin_b)
    : m_pin_a(pin_a)
    , m_pin_b(pin_b)
{
    m_listeners.push_back(m_pin_a.AttachListener([this](auto state) { Process(); }));
    m_listeners.push_back(m_pin_b.AttachListener([this](auto state) { Process(); }));
}

std::unique_ptr<ListenerCookie>
RotaryEncoder::AttachListener(std::function<void(Direction)> on_rotation)
{
    m_on_rotation = std::move(on_rotation);
    return std::make_unique<ListenerCookie>([this]() { m_on_rotation = [](auto) {}; });
}

void
RotaryEncoder::Process()
{
    auto pin_a = m_pin_a.GetState();
    auto pin_b = m_pin_b.GetState();

    // Get the current state of the rotary encoder
    m_state = kTable[m_state][(pin_a << 1) | pin_b];

    if (m_on_rotation)
    {
        if (m_state & kDirCw)
        {
            m_on_rotation(Direction::kRight);
        }
        else if (m_state & kDirCCw)
        {
            m_on_rotation(Direction::kLeft);
        }
    }
}
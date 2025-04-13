#include "encoder_input.hh"

#include <array>

using namespace hal;

namespace
{

class DummyListener : public IInput::IListener
{
public:
    void OnInput(const IInput::Event& event) final
    {
    }
};

DummyListener g_dummy_listener;

} // namespace


// TODO: Move to src, when the generic GPIO is used
EncoderInput::EncoderInput(RotaryEncoder& rotary_encoder,
                           ButtonDebouncer& button,
                           uint8_t switch_up_pin)
    : m_button(button)
    , m_pin_switch_up(static_cast<gpio_num_t>(switch_up_pin))
    , m_listener(&g_dummy_listener)
{
    m_rotary_listener = rotary_encoder.AttachListener([this](RotaryEncoder::Direction direction) {
        IInput::Event event = {
            .type = direction == RotaryEncoder::Direction::kLeft ? IInput::EventType::kLeft
                                                                 : IInput::EventType::kRight,
        };
        m_listener->OnInput(event);
    });

    m_button_listener = m_button.AttachListener([this](bool state) {
        if (m_listener)
        {
            IInput::Event event = {
                .type = state ? IInput::EventType::kButtonDown : IInput::EventType::kButtonUp,
            };
            m_listener->OnInput(event);
        }
    });
}

void
EncoderInput::AttachListener(IListener* listener)
{
    m_listener = listener;
}

IInput::State
EncoderInput::GetState()
{
    auto switch_up_active = gpio_get_level(m_pin_switch_up);
    auto button = m_button.GetState();

    uint8_t state = 0;

    if (switch_up_active)
    {
        state |= std::to_underlying(IInput::StateType::kSwitchUp);
    }
    if (button)
    {
        state |= std::to_underlying(IInput::StateType::kButtonDown);
    }

    return IInput::State(state);
}

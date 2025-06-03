#include "encoder_input.hh"

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


EncoderInput::EncoderInput(RotaryEncoder& rotary_encoder,
                           hal::IGpio& button)
    : m_button(button)
    , m_listener(&g_dummy_listener)
{
    m_rotary_listener =
        rotary_encoder.AttachIrqListener([this](RotaryEncoder::Direction direction) {
            IInput::Event event = {
                .type = direction == RotaryEncoder::Direction::kLeft ? IInput::EventType::kLeft
                                                                     : IInput::EventType::kRight,
            };
            m_listener->OnInput(event);
        });

    m_button_listener = m_button.AttachIrqListener([this](bool state) {
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
    auto button = m_button.GetState();

    uint8_t state = 0;

    if (button)
    {
        state |= std::to_underlying(IInput::StateType::kButtonDown);
    }

    return IInput::State(state);
}

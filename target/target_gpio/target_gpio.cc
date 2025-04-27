#include "target_gpio.hh"

TargetGpio::TargetGpio(uint8_t pin, TargetGpio::Polarity polarity)
    : m_pin(static_cast<gpio_num_t>(pin))
    , m_polarity(polarity == Polarity::kActiveHigh ? 0 : 1)
{
    gpio_isr_handler_add(m_pin, TargetGpio::StaticButtonIsr, static_cast<void*>(this));
}

void
TargetGpio::SetState(bool state)
{
    gpio_set_level(m_pin, state);
}

bool
TargetGpio::GetState() const
{
    return gpio_get_level(m_pin) ^ m_polarity;
}

std::unique_ptr<ListenerCookie>
TargetGpio::AttachIrqListener(std::function<void(bool)> on_state_change)
{
    m_on_state_change = std::move(on_state_change);

    return std::make_unique<ListenerCookie>([this]() { m_on_state_change = [](auto) {}; });
}

void
TargetGpio::ButtonIsr()
{
    auto state = GetState();
    m_on_state_change(state);
}

void
TargetGpio::StaticButtonIsr(void* arg)
{
    auto self = static_cast<TargetGpio*>(arg);
    self->ButtonIsr();
}

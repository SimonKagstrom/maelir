#include "target_gpio.hh"

TargetGpio::TargetGpio(uint8_t pin)
    : m_pin(static_cast<gpio_num_t>(pin))
{
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1 << m_pin;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_install_isr_service(0);

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
    return gpio_get_level(m_pin);
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

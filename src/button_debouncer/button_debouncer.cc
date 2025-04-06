// Based on https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
#include "button_debouncer.hh"

constexpr uint8_t kPressed = 0b11111111;
constexpr uint8_t kReleased = 0b00000000;
constexpr uint8_t kMaskBits = 0b11000111;

auto
Masked(uint8_t value)
{
    return value & kMaskBits;
}


ButtonDebouncer::ButtonDebouncer(hal::IGpio& button_gpio)
    : m_button_gpio(button_gpio)
{
    m_interrupt_listener = m_button_gpio.AttachIrqListener([this](bool state) {
        m_interrupt = true;
        Awake();
    });
}

std::unique_ptr<ListenerCookie>
ButtonDebouncer::AttachListener(std::function<void(bool)> on_state_change)
{
    m_on_state_change = std::move(on_state_change);

    return std::make_unique<ListenerCookie>([this]() { m_on_state_change = [](auto) {}; });
}

bool
ButtonDebouncer::GetState() const
{
    return m_state.load();
}

std::optional<milliseconds>
ButtonDebouncer::OnActivation()
{
    if (m_timer)
    {
        return std::nullopt;
    }

    if (!m_interrupt)
    {
        return std::nullopt;
    }

    m_timer = StartTimer(1ms, [this]() {
        m_interrupt = false;
        std::optional<milliseconds> out = 1ms;

        m_button_history <<= 1;
        m_button_history |= static_cast<int>(m_button_gpio.GetState());

        m_history_count++;

        if (m_history_count >= 8)
        {
            // Pressed?
            if (Masked(m_button_history) == 0b00000111)
            {
                m_button_history = kPressed;
                m_state = true;
                m_on_state_change(true);
            }
            else if (Masked(m_button_history) == 0b11000000)
            {
                m_button_history = kReleased;
                m_state = false;
                m_on_state_change(false);
            }

            m_history_count = 0;

            // Restart on the next interrupt
            m_timer = nullptr;
            out = std::nullopt;
        }

        return out;
    });

    return std::nullopt;
}

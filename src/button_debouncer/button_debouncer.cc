// Based on https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
#include "button_debouncer.hh"

constexpr uint8_t kPressed = 0b11111111;
constexpr uint8_t kReleased = 0b00000000;
constexpr uint8_t kMaskBits = 0b11000111;

namespace
{

auto
Masked(uint8_t value)
{
    return value & kMaskBits;
}

} // namespace


ButtonDebouncer::Button::Button(ButtonDebouncer& parent, std::unique_ptr<hal::IGpio> button_gpio)
    : m_parent(parent)
    , m_button_gpio(std::move(button_gpio))
{
    m_interrupt_listener = m_button_gpio->AttachIrqListener([this](bool state) {
        m_interrupt = true;
        m_parent.Awake();
    });
}

ButtonDebouncer::Button::~Button()
{
    m_parent.RemoveButton(this);
}

std::unique_ptr<ListenerCookie>
ButtonDebouncer::Button::AttachIrqListener(std::function<void(bool)> on_state_change)
{
    m_on_state_change = std::move(on_state_change);

    return std::make_unique<ListenerCookie>([this]() { m_on_state_change = [](auto) {}; });
}

bool
ButtonDebouncer::Button::GetState() const
{
    return m_state.load();
}

void
ButtonDebouncer::Button::ScanButton()
{
    if (m_timer)
    {
        return;
    }

    if (!m_interrupt)
    {
        return;
    }

    m_timer = m_parent.StartTimer(1ms, [this]() {
        m_interrupt = false;
        std::optional<milliseconds> out = 1ms;

        m_button_history <<= 1;
        m_button_history |= static_cast<int>(m_button_gpio->GetState());

        m_history_count++;

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

        if (m_history_count >= 8)
        {
            m_history_count = 0;

            // Restart on the next interrupt
            m_timer = nullptr;
            out = std::nullopt;
        }

        return out;
    });
}


std::unique_ptr<hal::IGpio>
ButtonDebouncer::AddButton(std::unique_ptr<hal::IGpio> pin)
{
    if (m_buttons.full())
    {
        return nullptr;
    }

    auto button = std::make_unique<Button>(*this, std::move(pin));

    m_buttons.push_back(button.get());

    return std::move(button);
}

void
ButtonDebouncer::RemoveButton(Button* button)
{
    auto it = std::find(m_buttons.begin(), m_buttons.end(), button);
    if (it != m_buttons.end())
    {
        m_buttons.erase(it);
    }
}

std::optional<milliseconds>
ButtonDebouncer::OnActivation()
{
    // Check all buttons, although probably only one has triggered
    for (auto button : m_buttons)
    {
        button->ScanButton();
    }

    return std::nullopt;
}

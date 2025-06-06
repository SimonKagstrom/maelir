#include "uart_event_listener.hh"

namespace
{
class DummyListener final : public hal::IInput::IListener
{
public:
    void OnInput(const hal::IInput::Event& event) override
    {
        // Do nothing
    }
};

DummyListener g_dummy_listener;

} // namespace

UartEventListener::UartEventListener(hal::IUart& uart)
    : m_uart(uart)
    , m_listener(&g_dummy_listener)
{
}

std::optional<milliseconds>
UartEventListener::OnActivation()
{
    auto s = m_uart.Read(m_buffer, 1h);

    m_deserializer.PushData(s);
    while (true)
    {
        auto v = m_deserializer.Deserialize();

        if (std::holds_alternative<hal::RawGpsData>(v))
        {
            const auto& gps_data = std::get<hal::RawGpsData>(v);

            m_gps_queue.push(gps_data);
            m_gps_semaphore.release();
        }
        else if (std::holds_alternative<serializer::InputEventState>(v))
        {
            auto event = std::get<serializer::InputEventState>(v);

            m_state = event.state;
            m_listener->OnInput(hal::IInput::Event {event.event});
        }
        else
        {
            // std::monostate
            break;
        }
    }

    return 0ms;
}


void
UartEventListener::AttachListener(hal::IInput::IListener* listener)
{
    m_listener = listener;
}

hal::IInput::State
UartEventListener::GetState()
{
    return m_state;
}


std::optional<hal::RawGpsData>
UartEventListener::WaitForData(os::binary_semaphore& semaphore)
{
    if (m_gps_queue.empty())
    {
        // Otherwise no need to wait
        m_gps_semaphore.acquire();
    }

    std::optional<hal::RawGpsData> data;
    if (hal::RawGpsData d; m_gps_queue.pop(d))
    {
        data = d;
    }

    semaphore.release();
    return data;
}

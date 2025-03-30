#include "uart_event_forwarder.hh"

#include "event_serializer.hh"

UartEventForwarder::UartEventForwarder(hal::IUart& send_uart,
                                       hal::IInput& input,
                                       GpsListener& gps_listener)
    : m_send_uart(send_uart)
    , m_input(input)
{
    m_input.AttachListener(this);

    gps_listener.AttachListener([this](hal::RawGpsData data) {
        m_gps_queue.push(data);
        Awake();
    });
}

std::optional<milliseconds>
UartEventForwarder::OnActivation()
{
    hal::IInput::Event event;
    hal::RawGpsData gps_data;

    m_buffer.clear();

    while (m_input_queue.pop(event))
    {
        auto d =
            serializer::Serialize(serializer::InputEventState {event.type, m_input.GetState()});

        std::ranges::copy(d, std::back_inserter(m_buffer));
    }

    while (m_gps_queue.pop(gps_data))
    {
        auto d = serializer::Serialize(gps_data);

        std::ranges::copy(d, std::back_inserter(m_buffer));
    }

    if (!m_buffer.empty())
    {
        // Send the data
        m_send_uart.Write(m_buffer);
    }

    return std::nullopt;
}

void
UartEventForwarder::OnInput(const hal::IInput::Event& event)
{
    m_input_queue.push(event);
    Awake();
}

#include "trip_computer.hh"

TripComputer::TripComputer(const MapMetadata& metadata,
                           ApplicationState& application_state,
                           std::unique_ptr<IGpsPort> gps_port,
                           std::unique_ptr<IRouteListener> route_listener)
    : m_application_state(application_state)
    , m_gps_port(std::move(gps_port))
    , m_route_listener(std::move(route_listener))
{
    m_gps_port->AwakeOn(GetSemaphore());
}

template <size_t Size>
bool
TripComputer::HistoryBuffer<Size>::Push(uint8_t value)
{
    if (m_history.full())
    {
        // Wrap around
        m_history[m_index] = value;
    }
    else
    {
        m_history.push_back(value);
    }

    m_index = (m_index + 1) % Size;

    return m_index == 0;
}

template <size_t Size>
uint8_t
TripComputer::HistoryBuffer<Size>::Average() const
{
    auto sum = std::accumulate(m_history.begin(), m_history.end(), 0);

    return static_cast<uint8_t>(sum / m_history.size());
}


std::optional<milliseconds>
TripComputer::OnActivation()
{
    if (auto gps_data = m_gps_port->Poll(); gps_data)
    {
        HandleSpeed(gps_data->speed);
    }

    return std::nullopt;
}

void
TripComputer::HandleSpeed(float speed_knots)
{
    if (speed_knots < 0)
    {
        return;
    }
    auto speed = static_cast<uint8_t>(std::clamp(speed_knots, 0.0f, 255.0f));

    auto wrap = m_second_history.Push(speed);
    auto second_average = m_second_history.Average();

    if (wrap)
    {
        // Wrap around, add to the minute history
        m_minute_history.Push(second_average);
    }

    auto state = m_application_state.Checkout();

    state->minute_average_speed = second_average;
    state->five_minute_average_speed = m_minute_history.Average();
}

template class TripComputer::HistoryBuffer<60>;
template class TripComputer::HistoryBuffer<5>;

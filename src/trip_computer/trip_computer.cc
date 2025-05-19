#include "trip_computer.hh"

TripComputer::TripComputer(ApplicationState& application_state,
                           std::unique_ptr<IGpsPort> gps_port,
                           std::unique_ptr<IRouteListener> route_listener,
                           float meters_per_pixel,
                           uint32_t land_mask_row_size)
    : m_application_state(application_state)
    , m_gps_port(std::move(gps_port))
    , m_route_listener(std::move(route_listener))
    , m_meters_per_pixel(meters_per_pixel)
    , m_land_mask_row_size(land_mask_row_size)
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
    if (m_history.empty())
    {
        return 0;
    }

    auto sum = std::accumulate(m_history.begin(), m_history.end(), 0);

    return static_cast<uint8_t>(sum / m_history.size());
}


std::optional<milliseconds>
TripComputer::OnActivation()
{
    if (auto ev = m_route_listener->Poll(); ev)
    {
        auto state = m_application_state.Checkout();

        // Reset
        m_current_route = {};
        m_passed_index = 0;

        state->route_total_meters = 0;
        state->route_passed_meters = 0;

        if (ev->type == IRouteListener::EventType::kReady)
        {
            m_current_route = ev->route;
            state->route_total_meters = MeasureRoute();
        }
    }

    if (auto gps_data = m_gps_port->Poll(); gps_data)
    {
        HandleSpeed(gps_data->speed);
        HandleRoute(gps_data->pixel_position);
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

    auto wrap = m_minute_history.Push(speed);
    auto minute_average = m_minute_history.Average();

    if (wrap)
    {
        // Wrap around, add to the minute history
        m_five_minute_history.Push(minute_average);
    }

    auto state = m_application_state.Checkout();

    state->minute_average_speed = minute_average;
    state->five_minute_average_speed = m_five_minute_history.Average();
}

void
TripComputer::HandleRoute(Point pixel_position)
{
    if (m_current_route.empty())
    {
        return;
    }

    auto route_iterator = RouteIterator(m_current_route, m_land_mask_row_size);
    auto last_point = route_iterator.Next();

    if (!last_point)
    {
        // Impossible case
        return;
    }

    uint32_t passed_distance = 0;
    auto index = 1;

    while (auto cur_point = route_iterator.Next())
    {
        constexpr auto kThreshold = 3 * kPathFinderTileSize;
        auto leg_distance = PointDistance(*last_point, *cur_point);

        passed_distance += leg_distance;
        if (std::abs(pixel_position.x - cur_point->x) < kThreshold &&
            std::abs(pixel_position.y - cur_point->y) < kThreshold)
        {
            // The boat is on the route
            if (m_passed_index < index)
            {
                m_passed_index = index;
                break;
            }
        }
        index++;
        last_point = cur_point;
    }
    auto state = m_application_state.Checkout();

    state->route_passed_meters = passed_distance;
}

uint32_t
TripComputer::MeasureRoute() const
{
    uint32_t meters = 0;

    auto route_iterator = RouteIterator(m_current_route, m_land_mask_row_size);
    auto last_point = route_iterator.Next();

    if (!last_point)
    {
        return 0;
    }

    while (auto cur_point = route_iterator.Next())
    {
        // Either diagonal, horizontal or vertical, so summing the pixel distance is OK
        auto distance = PointDistance(*last_point, *cur_point);

        meters += distance;

        last_point = cur_point;
    }

    return meters;
}

uint32_t
TripComputer::PointDistance(Point a, Point b) const
{
    auto vec = PointPairToVector(a, b);

    if (vec.IsDiagonal())
    {
        return std::abs(a.x - b.x) * m_meters_per_pixel;
    }

    return (std::abs(a.x - b.x) + std::abs(a.y - b.y)) * m_meters_per_pixel;
}

template class TripComputer::HistoryBuffer<60>;
template class TripComputer::HistoryBuffer<5>;

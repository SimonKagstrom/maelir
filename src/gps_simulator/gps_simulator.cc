#include "gps_simulator.hh"

#include "gps_utils.hh"
#include "route_utils.hh"

#include <cstdlib>

GpsSimulator::GpsSimulator(const MapMetadata& metadata,
                           ApplicationState& application_state,
                           RouteService& route_service)
    : m_map_metadata(metadata)
    , m_application_state(application_state)
    , m_route_service(route_service)
{
    m_application_state_listener = m_application_state.AttachListener(GetSemaphore());

    m_route_listener = m_route_service.AttachListener();
    m_route_listener->AwakeOn(GetSemaphore());
}

std::optional<milliseconds>
GpsSimulator::OnActivation()
{
    while (auto route = m_route_listener->Poll())
    {
        if (route->type == IRouteListener::EventType::kReady)
        {
            m_route_iterator = nullptr;
            m_next_position = std::nullopt;
            m_route.clear();
            std::ranges::copy(route->route, std::back_inserter(m_route));
            m_route_iterator = m_route_service.CreateRouteIterator(m_route);
            if (auto pos = m_route_iterator->Next(); pos.has_value())
            {
                m_position = *pos;
                m_next_position = m_route_iterator->Next();
            }

            if (m_next_position)
            {
                m_direction = PointPairToVector(m_position, *m_next_position);
                m_angle = m_direction.Angle();
                m_speed = 20;
            }
            m_route_pending = false;
        }
        else
        {
            m_route_pending = true;
            m_route_iterator = nullptr;
            m_route.clear();
            m_next_position = std::nullopt;
        }
    }

    auto application_state = m_application_state.CheckoutReadonly();

    auto before = m_state;
    do
    {
        before = m_state;
        switch (m_state)
        {
        case State::kIdle:
            if (application_state->demo_mode)
            {
                m_state = State::kRequestRoute;
                break;
            }

            return std::nullopt;

        case State::kRequestRoute:
            if (m_route_pending)
            {
                m_state = State::kWaitForRoute;
                break;
            }
            else
            {
                auto from = m_route_service.RandomWaterPoint();
                auto to = m_route_service.RandomWaterPoint();

                printf("Request route from %d %d to %d %d\n", from.x, from.y, to.x, to.y);
                m_route_service.RequestRoute(from, to);
                m_state = State::kWaitForRoute;
                m_route_pending = true;
            }
            break;

        case State::kWaitForRoute:
            if (application_state->demo_mode == false)
            {
                m_state = State::kIdle;
                break;
            }
            if (!m_route_pending && m_route_iterator)
            {
                m_state = State::kDemo;
                break;
            }
            return std::nullopt;

        case State::kDemo:
            if (application_state->demo_mode == false)
            {
                m_state = State::kIdle;
                break;
            }
            if (!m_route_iterator || m_next_position == std::nullopt)
            {
                m_state = State::kRequestRoute;
                break;
            }

            RunDemo();

            return 82ms + milliseconds(m_speed);

        case State::kValueCount:
            break;
        }

        //    if (before != m_state)
        //    printf("ST now %d\n", before);

    } while (m_state != before);

    return std::nullopt;
}

hal::RawGpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();

    auto position = gps::PointToPosition(m_map_metadata, m_position);

    semaphore.release();

    return {position, m_angle, m_speed};
}

void
GpsSimulator::RunDemo()
{
    auto target_angle = m_direction.Angle();
    if (m_angle != target_angle)
    {
        auto diff = target_angle - m_angle;
        if (diff > 180)
        {
            diff -= 360;
        }
        else if (diff < -180)
        {
            diff += 360;
        }

        auto step = std::min(3, std::abs(diff));
        if (diff < 0)
        {
            step = -step;
        }

        m_angle += step;
    }

    if (m_position == m_next_position)
    {
        m_next_position = m_route_iterator->Next();
        if (m_next_position)
        {
            m_direction = PointPairToVector(m_position, *m_next_position);
            m_speed = 20 + rand() % 10;
        }
        else
        {
            m_route_iterator = nullptr;
        }
    }

    m_position = m_position + m_direction;
    m_has_data_semaphore.release();
}

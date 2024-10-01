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
    m_route_listener = m_route_service.AttachListener();
    m_route_listener->AwakeOn(GetSemaphore());
}

std::optional<milliseconds>
GpsSimulator::OnActivation()
{
    if (auto route = m_route_listener->Poll())
    {
        if (route->type == IRouteListener::EventType::kRouteReady)
        {
            m_route_iterator = nullptr;
            m_next_position = std::nullopt;
            m_route_iterator = m_route_service.CreateRouteIterator(route->route);
            if (auto pos = m_route_iterator->Next(); pos.has_value())
            {
                m_position = *pos;
                m_next_position = m_route_iterator->Next();
            }

            if (m_next_position)
            {
                m_direction = PointPairToVector(m_position, *m_next_position);
                m_speed = 20;
            }

            m_waiting_for_route = false;
        }
    }

    if (!m_waiting_for_route && (!m_route_iterator || !m_next_position))
    {
        auto from = m_route_service.RandomWaterPoint();
        auto to = m_route_service.RandomWaterPoint();

        printf("Request route from %d %d to %d %d\n", from.x, from.y, to.x, to.y);
        m_route_service.RequestRoute(from, to);

        m_waiting_for_route = true;
        return std::nullopt;
    }

    if (!m_next_position)
    {
        //        auto from = m_route_service.RandomWaterPoint();
        //        auto to = m_route_service.RandomWaterPoint();
        //
        //        printf("Route from %d %d to %d %d\n", from.x, from.y, to.x, to.y);
        //        m_route_service.RequestRoute(from, to);

        return std::nullopt;
    }

    if (m_position == m_next_position)
    {
        m_next_position = m_route_iterator->Next();
        if (m_next_position)
        {
            m_direction = PointPairToVector(m_position, *m_next_position);
            m_speed = 20 + rand() % 10;
        }
    }

    m_position = m_position + m_direction;
    m_has_data_semaphore.release();

    return 100ms + milliseconds(m_speed);
}

hal::RawGpsData
GpsSimulator::WaitForData(os::binary_semaphore& semaphore)
{
    m_has_data_semaphore.acquire();

    auto position = gps::PointToPosition(m_map_metadata, m_position);
    auto vobb = gps::PositionToPoint(m_map_metadata, position);

    auto angle = m_direction.Angle();

    semaphore.release();

    return {position, angle, m_speed};
}

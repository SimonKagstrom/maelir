#include "route_service.hh"

#include "route_utils.hh"

class RouteService::RouteListenerImpl : public IRouteListener
{
public:
    void PushEvent(IRouteListener::EventType event, std::span<const IndexType> route)
    {
        m_events.push({event, route});
        if (m_semaphore)
        {
            m_semaphore->release();
        }
    }

private:
    void AwakeOn(os::binary_semaphore& semaphore) final
    {
        m_semaphore = &semaphore;
    }

    std::optional<IRouteListener::Event> Poll() final
    {
        IRouteListener::Event ev;

        if (m_events.pop(ev))
        {
            return ev;
        }

        return std::nullopt;
    }

    os::binary_semaphore* m_semaphore {nullptr};
    etl::queue_spsc_atomic<IRouteListener::Event, 4> m_events;
};


RouteService::RouteService(const MapMetadata& metadata)
    : m_row_size(metadata.land_mask_row_size)
    , m_rows(metadata.land_mask_rows)
{
    const auto land_mask = reinterpret_cast<const uint32_t*>(
        reinterpret_cast<const uint8_t*>(&metadata) + metadata.land_mask_data_offset);

    m_router = std::make_unique<Router<kTargetCacheSize>>(
        std::span<const uint32_t> {land_mask,
                                   metadata.land_mask_row_size * metadata.land_mask_rows},
        metadata.land_mask_rows,
        metadata.land_mask_row_size);
}

void
RouteService::RequestRoute(Point from, Point to)
{
    // Context: Another thread
    m_requested_route.push(
        std::make_pair(PointToLandIndex(from, m_row_size), PointToLandIndex(to, m_row_size)));

    Awake();
}

std::unique_ptr<IRouteListener>
RouteService::AttachListener()
{
    assert(!m_listeners.full());

    auto out = std::make_unique<RouteService::RouteListenerImpl>();
    m_listeners.push_back(out.get());

    return out;
}

std::optional<milliseconds>
RouteService::OnActivation()
{
    std::pair<IndexType, IndexType> route_request;

    if (m_requested_route.pop(route_request))
    {
        auto [from, to] = route_request;

        for (auto listener : m_listeners)
        {
            listener->PushEvent(IRouteListener::EventType::kRouteCalculating, {});
        }
        auto route = m_router->CalculateRoute(from, to);

        for (auto listener : m_listeners)
        {
            listener->PushEvent(IRouteListener::EventType::kRouteReady, route);
        }
    }

    return std::nullopt;
}

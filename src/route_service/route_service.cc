#include "route_service.hh"

#include "route_utils.hh"

#include <cstdlib>

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
    // Copy the land mask to PSRAM for faster access (~330KiB)
    m_land_mask.resize((m_rows * m_row_size) / 32);
    auto p = reinterpret_cast<const uint8_t*>(&metadata) + metadata.land_mask_data_offset;
    memcpy(m_land_mask.data(), p, m_land_mask.size() * sizeof(uint32_t));

    m_router = std::make_unique<Router<kTargetCacheSize>>(
        m_land_mask, metadata.land_mask_rows, metadata.land_mask_row_size);
}

void
RouteService::RequestRoute(Point from, Point to)
{
    m_calculating_semaphore.acquire();

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
            listener->PushEvent(IRouteListener::EventType::kCalculating, {});
        }
        auto route = m_router->CalculateRoute(from, to);

        for (auto listener : m_listeners)
        {
            listener->PushEvent(IRouteListener::EventType::kReady, route);
        }
        m_calculating_semaphore.release();
    }

    return std::nullopt;
}

Point
RouteService::RandomWaterPoint() const
{
    Point p;
    do
    {
        p = {static_cast<int32_t>((rand() % m_row_size) * kPathFinderTileSize),
             static_cast<int32_t>((rand() % m_rows) * kPathFinderTileSize)};
    } while (IsWater(m_land_mask, PointToLandIndex(p, m_row_size)) == false);

    return p;
}

std::unique_ptr<RouteIterator>
RouteService::CreateRouteIterator(std::span<const IndexType> route) const
{
    return std::make_unique<RouteIterator>(route, m_row_size);
}

#pragma once

#include "base_thread.hh"
#include "i_route_listener.hh"
#include "route_iterator.hh"
#include "tile.hh"

#include <etl/mutex.h>
#include <etl/queue_spsc_atomic.h>
#include <etl/vector.h>

class RouteService : public os::BaseThread
{
public:
    RouteService(const MapMetadata& metadata);

    // Context: Another thread
    void RequestRoute(Point from, Point to);

    // Helper to create a route iterator
    std::unique_ptr<RouteIterator> CreateRouteIterator(std::span<const IndexType> route) const;

    // Helper to get a random point for water (demo mode)
    Point RandomWaterPoint() const;

    uint32_t GetRowSize() const
    {
        return m_row_size;
    }

    std::unique_ptr<IRouteListener> AttachListener();

private:
    class RouteListenerImpl;

    std::optional<milliseconds> OnActivation() final;


    const uint32_t m_row_size;
    const uint32_t m_rows;
    std::vector<uint32_t> m_land_mask;

    os::binary_semaphore m_calculating_semaphore {1};
    etl::queue_spsc_atomic<std::pair<IndexType, IndexType>, 8> m_requested_route;
    etl::vector<RouteListenerImpl*, 4> m_listeners;

    // Unique, to place this class in PSRAM
    std::unique_ptr<Router<kTargetCacheSize>> m_router;
};

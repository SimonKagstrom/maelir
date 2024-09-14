#pragma once

#include "base_thread.hh"
#include "i_route_listener.hh"

#include <etl/mutex.h>
#include <etl/queue_spsc_atomic.h>
#include <etl/vector.h>

class RouteService : public os::BaseThread
{
public:
    RouteService();

    // Context: Another thread
    void RequestRoute(Point from, Point to);

    std::unique_ptr<IRouteListener> AttachListener();

private:
    class RouteListenerImpl;


    std::optional<milliseconds> OnActivation() final;

    etl::mutex m_lock;
    etl::queue_spsc_atomic<std::pair<IndexType, IndexType>, 8> m_requested_route;
    etl::vector<RouteListenerImpl*, 4> m_listeners;

    // Unique, to place this class in PSRAM
    std::unique_ptr<Router<kTargetCacheSize>> m_router;
};

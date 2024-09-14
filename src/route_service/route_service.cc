#include "route_service.hh"

#include "generated_land_mask.hh"

class RouteService::RouteListenerImpl : public IRouteListener
{
private:
    void AwakeOn(os::binary_semaphore& semaphore) final
    {
    }

    std::optional<const IndexType> Poll() final
    {
        return std::nullopt;
    }
};


RouteService::RouteService()
{
    m_router = std::make_unique<Router<kTargetCacheSize>>(
        std::span<const uint32_t> {kLandMask, kLandMaskRowSize * kLandMaskRows},
        kLandMaskRowSize / kLandMaskRows,
        kLandMaskRowSize);
}

void
RouteService::RequestRoute(Point from, Point to)
{
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
    return std::nullopt;
}

#pragma once

#include "router.hh"
#include "semaphore.hh"

#include <optional>
#include <span>

class IRouteListener
{
public:
    enum class EventType
    {
        kRouteCalculating,
        kRouteReady,

        kValueCount,
    };

    struct Event
    {
        EventType type;

        // Valid if kRouteReady (copy it)
        std::span<const IndexType> route;
    };


    virtual ~IRouteListener() = default;

    virtual void AwakeOn(os::binary_semaphore& semaphore) = 0;

    virtual std::optional<Event> Poll() = 0;
};
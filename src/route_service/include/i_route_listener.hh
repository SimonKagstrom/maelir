#pragma once

#include "event_notifier.hh"
#include "router.hh"

#include <optional>
#include <span>

class IRouteListener
{
public:
    enum class EventType
    {
        kCalculating, // New route being calculated
        kReady,       // The route is ready
        kReleased,    // The route is released (dropped)

        kValueCount,
    };

    struct Event
    {
        EventType type;

        // Valid if kRouteReady (copy it)
        std::span<const IndexType> route;
    };


    virtual ~IRouteListener() = default;

    virtual void AwakeOn(IEventNotifier& notifier) = 0;

    virtual std::optional<Event> Poll() = 0;
};

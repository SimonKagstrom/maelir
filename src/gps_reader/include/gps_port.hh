#pragma once

#include "application_state.hh"
#include "event_notifier.hh"
#include "hal/i_gps.hh"

#include <optional>

class IGpsPort
{
public:
    virtual ~IGpsPort() = default;

    void AwakeOn(IEventNotifier& notifier)
    {
        DoAwakeOn(&notifier);
    }

    void DisableWakeup()
    {
        DoAwakeOn(nullptr);
    }

    virtual std::optional<GpsData> Poll() = 0;

protected:
    virtual void DoAwakeOn(IEventNotifier* notifier) = 0;
};

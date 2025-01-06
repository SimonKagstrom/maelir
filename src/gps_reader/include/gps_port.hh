#pragma once

#include "gps_data.hh"
#include "semaphore.hh"

#include <optional>

class IGpsPort
{
public:
    virtual ~IGpsPort() = default;

    void AwakeOn(os::binary_semaphore& semaphore)
    {
        DoAwakeOn(&semaphore);
    }

    void DisableWakeup()
    {
        DoAwakeOn(nullptr);
    }

    virtual std::optional<GpsData> Poll() = 0;

protected:
    virtual void DoAwakeOn(os::binary_semaphore* semaphore) = 0;
};

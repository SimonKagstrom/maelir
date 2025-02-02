#pragma once

#include "hal/i_gps.hh"
#include "semaphore.hh"

#include <optional>

struct GpsData
{
    GpsPosition position;
    Point pixel_position;

    float speed;
    float heading;

    // Add time, height, etc.
};

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

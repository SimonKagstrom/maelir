#pragma once

#include "gps_data.hh"
#include "semaphore.hh"

#include <optional>

class IGpsPort
{
public:
    virtual ~IGpsPort() = default;

    virtual void AwakeOn(os::binary_semaphore& semaphore) = 0;

    virtual std::optional<GpsData> Poll() = 0;
};

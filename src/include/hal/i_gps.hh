#pragma once

#include "gps_data.hh"
#include "semaphore.hh"

#include <memory>

namespace hal
{

class IGps
{
public:
    virtual ~IGps() = default;

    /// @brief block waiting for data and signal the semaphore when it arrives
    virtual GpsData WaitForData(os::binary_semaphore& semaphore) = 0;
};

} // namespace hal

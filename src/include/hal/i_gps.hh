#pragma once

#include "gps_data.hh"
#include "semaphore.hh"

#include <memory>

namespace hal
{

struct RawGpsData
{
    std::optional<GpsPosition> position;
    std::optional<float> heading;
    std::optional<float> speed;
};

class IGps
{
public:
    virtual ~IGps() = default;

    /// @brief block waiting for data and signal the semaphore when it arrives
    virtual RawGpsData WaitForData(os::binary_semaphore& semaphore) = 0;
};

} // namespace hal

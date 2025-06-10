#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace hal
{

class IOtaUpdater
{
public:
    virtual ~IOtaUpdater() = default;

    /// Return if the current application is newly updated
    virtual bool ApplicationHasBeenUpdated() const = 0;

    /// Mark the currently running application as valid (disable rollback)
    virtual void MarkApplicationAsValid() = 0;

    /// Return the Wifi SSID of the device
    virtual const char* GetSsid() = 0;

    /// Perform the update. This might be a blocking call. The progress is reported in percent
    virtual void Update(std::function<void(uint8_t)> progress) = 0;
};

} // namespace hal

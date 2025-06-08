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

    virtual void Setup(std::function<void(uint8_t)> progress) = 0;
};

} // namespace hal

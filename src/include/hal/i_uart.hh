#pragma once
#include "time.hh"

#include <cstdint>
#include <span>

namespace hal
{

class IUart
{
public:
    virtual ~IUart() = default;

    /// Blocking send of data
    virtual void Write(std::span<const uint8_t> data) = 0;

    /**
     * @brief Blocking read of data
     *
     * @param data the buffer to read into
     * @param timeout the timeout for the read operation
     *
     * @return a span to the read data, which refers to the input parameter
     */
    virtual std::span<uint8_t> Read(std::span<uint8_t> data, milliseconds timeout) = 0;
};

} // namespace hal

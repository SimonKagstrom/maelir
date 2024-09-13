#pragma once

#include "image.hh"
#include "rect.hh"
#include "tile.hh"

#include <optional>

namespace hal
{

constexpr auto kDisplayWidth = 720;
constexpr auto kDisplayHeight = 720;

class IDisplay
{
public:
    virtual ~IDisplay() = default;

    /**
     * @brief Return a pointer to the frame buffer.
     *
     * @note only valid until @a Flip is called.
     *
     * @return the frame buffer
     */
    virtual uint16_t* GetFrameBuffer() = 0;

    // Inspired by SDL2
    virtual void Flip() = 0;
};

} // namespace hal

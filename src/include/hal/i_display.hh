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

    // Inspired by SDL2
    virtual void Blit(const Image& image, Rect to, std::optional<Rect> from = std::nullopt) = 0;

    // Black is translucent in the image to write
    virtual void AlphaBlit(const Image& image,
                           uint8_t alpha_byte,
                           Rect to,
                           std::optional<Rect> from = std::nullopt) = 0;

    virtual void
    DrawAlphaLine(Point from, Point to, uint8_t width, uint16_t rgb565, uint8_t alpha_byte) = 0;

    virtual void Flip() = 0;
};

} // namespace hal

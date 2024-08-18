#pragma once

#include "image.hh"
#include "rect.hh"

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
    virtual void Blit(const Image &image, Rect to, std::optional<Rect> from = std::nullopt) = 0;

    virtual void Flip() = 0;
};

}
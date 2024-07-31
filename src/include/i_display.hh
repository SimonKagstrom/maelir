#pragma once

#include "image.hh"
#include "rect.hh"

namespace hal
{

constexpr auto kDisplayWidth = 480;
constexpr auto kDisplayHeight = 480;

class IDisplay
{
public:
    virtual ~IDisplay() = default;

    // Inspired by SDL2
    virtual void Blit(const Image &image, Rect from, Rect to) = 0;

    virtual void Flip() = 0;
};

}
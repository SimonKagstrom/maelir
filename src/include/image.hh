#pragma once

#include <cstdint>
#include <span>

class Image
{
public:
    Image(std::span<const uint16_t> data, uint16_t width, uint16_t height)
        : data(data)
        , width(width)
        , height(height)
    {
    }

    std::span<const uint16_t> data;
    uint16_t width;
    uint16_t height;
};

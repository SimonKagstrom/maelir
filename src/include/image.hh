#pragma once

#include <cstdint>
#include <span>

struct Image
{
    std::span<const uint16_t> data;
    uint16_t width;
    uint16_t height;
};
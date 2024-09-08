#pragma once

#include <optional>
#include <cstdint>

struct GpsPosition
{
    double latitude;
    double longitude;
};

struct PixelPosition
{
    int32_t x;
    int32_t y;
};

struct GpsData
{
    GpsPosition position;
    PixelPosition pixel_position;

    float speed;
    float heading;

    // Add time, height, etc.
};

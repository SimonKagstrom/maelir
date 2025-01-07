#pragma once

#include "tile.hh"

#include <cstdint>
#include <optional>

struct GpsPosition
{
    float latitude;
    float longitude;

    bool operator==(const GpsPosition& other) const = default;
};
static_assert(sizeof(GpsPosition) == 8);

struct GpsData
{
    GpsPosition position;
    Point pixel_position;

    float speed;
    float heading;

    // Add time, height, etc.
};

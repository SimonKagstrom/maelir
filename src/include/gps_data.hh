#pragma once

#include "tile.hh"

#include <cstdint>
#include <optional>

struct GpsPosition
{
    double latitude;
    double longitude;
};

struct GpsData
{
    GpsPosition position;
    Point pixel_position;

    float speed;
    float heading;

    // Add time, height, etc.
};

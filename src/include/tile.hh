#pragma once

#include <cstdint>

constexpr auto kTileSize = 240;
constexpr auto kPathFinderTileSize = kTileSize / 10;

struct Point
{
    int32_t x;
    int32_t y;
};

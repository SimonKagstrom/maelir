#pragma once

#include <cstdint>

constexpr auto kTileSize = 240;
constexpr auto kPathFinderTileSize = kTileSize / 10;

struct Point
{
    int32_t x;
    int32_t y;
};

struct Direction
{
    int8_t dx;
    int8_t dy;

    static consteval Direction StandStill()
    {
        return {0, 0};
    }
};

inline auto
operator==(const Direction& lhs, const Direction& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}
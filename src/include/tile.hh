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

    static consteval auto Standstill()
    {
        return Direction {0, 0};
    };

    static consteval auto Up()
    {
        return Direction {0, -1};
    };

    static consteval auto Down()
    {
        return Direction {0, 1};
    };

    static consteval auto Left()
    {
        return Direction {-1, 0};
    };

    static consteval auto Right()
    {
        return Direction {1, 0};
    };

    static consteval auto UpLeft()
    {
        return Direction {-1, -1};
    };

    static consteval auto UpRight()
    {
        return Direction {1, -1};
    };

    static consteval auto DownLeft()
    {
        return Direction {-1, 1};
    };

    static consteval auto DownRight()
    {
        return Direction {1, 1};
    };
};

inline auto
operator==(const Direction& lhs, const Direction& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}
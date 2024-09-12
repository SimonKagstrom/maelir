#pragma once

#include <algorithm>
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

    Direction Perpendicular()
    {
        return Direction {static_cast<int8_t>(-dy), dx};
    }

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
PointPairToDirection(Point from, Point to)
{
    int8_t dx = std::clamp(to.x - from.x, -1, 1);
    int8_t dy = std::clamp(to.y - from.y, -1, 1);

    return Direction {dx, dy};
}

inline auto
operator==(const Point& lhs, const Point& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline auto
operator==(const Direction& lhs, const Direction& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}

inline auto
operator*(Direction lhs, int rhs)
{
    return Direction {static_cast<int8_t>(lhs.dx * rhs), static_cast<int8_t>(lhs.dy * rhs)};
}

inline auto
operator+(Point lhs, Direction rhs)
{
    return Point {lhs.x + rhs.dx, lhs.y + rhs.dy};
}

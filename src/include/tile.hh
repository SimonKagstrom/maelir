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

struct Vector
{
    int8_t dx;
    int8_t dy;

    bool IsDiagonal() const
    {
        return dx != 0 && dy != 0;
    }

    Vector Perpendicular() const
    {
        return Vector {static_cast<int8_t>(-dy), dx};
    }

    static consteval auto Standstill()
    {
        return Vector {0, 0};
    };

    static consteval auto Up()
    {
        return Vector {0, -1};
    };

    static consteval auto Down()
    {
        return Vector {0, 1};
    };

    static consteval auto Left()
    {
        return Vector {-1, 0};
    };

    static consteval auto Right()
    {
        return Vector {1, 0};
    };

    static consteval auto UpLeft()
    {
        return Vector {-1, -1};
    };

    static consteval auto UpRight()
    {
        return Vector {1, -1};
    };

    static consteval auto DownLeft()
    {
        return Vector {-1, 1};
    };

    static consteval auto DownRight()
    {
        return Vector {1, 1};
    };
};

inline auto
PointPairToDirection(Point from, Point to)
{
    int8_t dx = std::clamp(to.x - from.x, static_cast<int32_t>(-1), static_cast<int32_t>(1));
    int8_t dy = std::clamp(to.y - from.y, static_cast<int32_t>(-1), static_cast<int32_t>(1));

    return Vector {dx, dy};
}

inline auto
operator==(const Point& lhs, const Point& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline auto
operator==(const Vector& lhs, const Vector& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}

inline auto
operator*(Vector lhs, int rhs)
{
    return Vector {static_cast<int8_t>(lhs.dx * rhs), static_cast<int8_t>(lhs.dy * rhs)};
}

inline auto
operator+(Point lhs, Vector rhs)
{
    return Point {lhs.x + rhs.dx, lhs.y + rhs.dy};
}

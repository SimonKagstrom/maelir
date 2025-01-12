#pragma once

#include <algorithm>
#include <cstdint>

#include <cstddef>

constexpr auto kGpsPositionSize = 256;

constexpr auto kTileSize = 240;
constexpr auto kPathFinderTileSize = kTileSize / 10;

// For the path finder
using IndexType = uint32_t;
using CostType = uint32_t;

// TILRSWFT
constexpr auto kMetadataMagic = 0x54494C5253574654ull;

struct FlashTile
{
    uint32_t size;
    uint32_t flash_offset;
};
static_assert(sizeof(FlashTile) == 8);

struct MapGpsRasterTile
{
    float latitude;
    float longitude;

    float latitude_offset;
    float longitude_offset;
};

struct MapMetadata
{
    uint64_t magic;

    float lowest_longitude;
    float lowest_latitude;
    float highest_longitude;
    float highest_latitude;

    uint32_t tile_count;
    uint32_t tile_row_size;
    uint32_t tile_rows;

    uint32_t land_mask_row_size;
    uint32_t land_mask_rows;

    uint32_t gps_data_row_size;
    uint32_t gps_data_rows;

    // Offset from the start of the metadata
    uint32_t tile_data_offset;
    uint32_t land_mask_data_offset;
    uint32_t gps_position_offset;
};
static_assert(offsetof(MapMetadata, tile_count) == 24);
static_assert(offsetof(MapMetadata, land_mask_data_offset) == 56);
static_assert(sizeof(MapMetadata) == 64);

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

    int Angle() const
    {
        if (dx == 0 && dy == -1)
        {
            return 0;
        }
        if (dx == 1 && dy == -1)
        {
            return 45;
        }
        if (dx == 1 && dy == 0)
        {
            return 90;
        }
        if (dx == 1 && dy == 1)
        {
            return 135;
        }
        if (dx == 0 && dy == 1)
        {
            return 180;
        }
        if (dx == -1 && dy == 1)
        {
            return 225;
        }
        if (dx == -1 && dy == 0)
        {
            return 270;
        }
        if (dx == -1 && dy == -1)
        {
            return 315;
        }

        return 0;
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
PointPairToVector(Point from, Point to)
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
operator/(const Point& lhs, auto rhs)
{
    return Point {lhs.x / rhs, lhs.y / rhs};
}

inline auto
operator-(const Point& lhs, auto rhs)
{
    return Point {lhs.x - rhs.x, lhs.y - rhs.y};
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

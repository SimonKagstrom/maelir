#pragma once

#include "gps_data.hh"
#include "tile.hh"

#include <optional>
#include <utility>

constexpr std::optional<unsigned>
PointToTileIndex(uint32_t x, uint32_t y)
{
    if (x > kTileSize * kRowSize)
    {
        return std::nullopt;
    }

    if (y > kTileSize * kColumnSize)
    {
        return std::nullopt;
    }

    return (y / kTileSize) * kRowSize + x / kTileSize;
}

constexpr std::pair<uint32_t, uint32_t>
PositionToPoint(const auto& GpsData)
{
    auto longitude_offset = GpsData.longitude - kCornerLongitude;
    auto latitude_offset = GpsData.latitude - kCornerLatitude;

    uint32_t x = longitude_offset * kPixelLongitudeSize;
    uint32_t y = latitude_offset * kPixelLatitudeSize;

    if (longitude_offset < 0)
    {
        x = 0;
    }
    if (latitude_offset < 0)
    {
        y = 0;
    }

    if (x >= kTileSize * kRowSize - kTileSize)
    {
        x = kTileSize * kRowSize - kTileSize;
    }
    if (y >= kTileSize * kColumnSize - kTileSize)
    {
        y = kTileSize * kColumnSize - kTileSize;
    }

    return {x, y};
}

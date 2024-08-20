#pragma once

#include "gps_data.hh"
#include "hal/i_display.hh"
#include "tile.hh"

#include <optional>
#include <utility>

constexpr std::optional<unsigned>
PointToTileIndex(uint32_t x, uint32_t y)
{
    if (x >= kTileSize * kRowSize)
    {
        return std::nullopt;
    }

    if (y >= kTileSize * kColumnSize)
    {
        return std::nullopt;
    }

    return (y / kTileSize) * kRowSize + x / kTileSize;
}

constexpr std::pair<int32_t, int32_t>
PositionToPoint(const auto& gps_data)
{
    auto longitude_offset = gps_data.longitude - kCornerLongitude;
    auto latitude_offset = gps_data.latitude - kCornerLatitude;

    int32_t x = longitude_offset * kPixelLongitudeSize;
    int32_t y = latitude_offset * kPixelLatitudeSize;

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

constexpr std::pair<int32_t, int32_t>
PositionToPointCenteredAndClamped(const auto& gps_data)
{
    auto [x, y] = PositionToPoint(gps_data);

    x = std::clamp(static_cast<int>(x - hal::kDisplayWidth / 2),
                   0,
                   (kRowSize - 1) * kTileSize - hal::kDisplayWidth / 2);
    y = std::clamp(static_cast<int>(y - hal::kDisplayHeight / 2),
                   0,
                   (kColumnSize - 1) * kTileSize - hal::kDisplayHeight / 2);

    return {x, y};
}

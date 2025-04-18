#pragma once

#include "generated_tiles.hh"
#include "hal/i_display.hh"
#include "hal/i_gps.hh"
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

constexpr Point
PositionToMapCenter(const auto& pixel_position)
{
    auto x = pixel_position.x;
    auto y = pixel_position.y;

    x = std::clamp(
        static_cast<int>(x - hal::kDisplayWidth / 2), 0, kRowSize * kTileSize - hal::kDisplayWidth);
    y = std::clamp(static_cast<int>(y - hal::kDisplayHeight / 2),
                   0,
                   kColumnSize * kTileSize - hal::kDisplayHeight);

    return {x, y};
}

#pragma once

#include "tile.hh"

#include <optional>

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

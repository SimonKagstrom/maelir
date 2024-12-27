#include "position_converter.hh"

#include <cassert>
#include <fmt/format.h>
#include <span>

namespace
{

auto
PositionSpanFromMetadata(const MapMetadata& metadata)
{
    const auto base = reinterpret_cast<const uint8_t*>(&metadata);
    return std::span<const MapGpsRasterTile> {
        reinterpret_cast<const MapGpsRasterTile*>(base + metadata.gps_position_offset),
        metadata.gps_data_rows * metadata.gps_data_row_size};
}

Point
MapGpsTileToPoint(const MapGpsRasterTile& tile, const GpsPosition& gps_data)
{
    float diff_longitude = tile.longitude_offset;
    float diff_latitude = tile.latitude_offset;

    auto out =
        Point {static_cast<int32_t>(((gps_data.longitude - tile.longitude) / diff_longitude) *
                                    kGpsPositionSize),
               static_cast<int32_t>(((gps_data.latitude - tile.latitude) / diff_latitude) *
                                    kGpsPositionSize)};

    return out;
}


std::optional<Point>
PositionIsInTile(const MapGpsRasterTile& cur, int32_t x, int32_t y, const GpsPosition& gps_data)
{
    if (gps_data.longitude >= cur.longitude &&
        gps_data.longitude <= cur.longitude + cur.longitude_offset &&
        gps_data.latitude <= cur.latitude &&
        gps_data.latitude >= cur.latitude + cur.latitude_offset)
    {
        float diff_longitude = cur.longitude_offset;
        float diff_latitude = cur.latitude_offset;

        auto out = Point {
            x * kGpsPositionSize +
                static_cast<int32_t>(((gps_data.longitude - cur.longitude) / diff_longitude) *
                                     kGpsPositionSize),
            y * kGpsPositionSize +
                static_cast<int32_t>(
                    ((gps_data.latitude - cur.latitude + cur.latitude_offset) / diff_latitude) *
                    kGpsPositionSize)};

        //        fmt::print("Found in tile {} ({},{}) -> {},{}\n", cur_index, x, y, out.x, out.y);

        return out;
    }

    return std::nullopt;
}

struct CachedGpsTile
{
    MapGpsRasterTile tile;
    int32_t x;
    int32_t y;
};

static CachedGpsTile g_current_tile {{0, 0, 0, 0}, 0, 0};

} // namespace


namespace gps
{

Point
PositionToPoint(const MapMetadata& metadata, const GpsPosition& gps_data)
{
    // First look in the cached tile (where the boat was last time)
    if (auto point =
            PositionIsInTile(g_current_tile.tile, g_current_tile.x, g_current_tile.y, gps_data);
        point)
    {
        return *point;
    }

    if (gps_data.longitude < metadata.lowest_longitude ||
        gps_data.longitude > metadata.highest_longitude ||
        gps_data.latitude < metadata.lowest_latitude ||
        gps_data.latitude > metadata.highest_latitude)
    {
        return {0, 0};
    }

    auto positions = PositionSpanFromMetadata(metadata);

    float diff_latitude = metadata.highest_latitude - metadata.lowest_latitude;
    float diff_longitude = metadata.highest_longitude - metadata.lowest_longitude;

    int tile_x = ((gps_data.longitude - metadata.lowest_longitude) / diff_longitude) *
                 metadata.gps_data_row_size;
    int tile_y =
        ((metadata.highest_latitude - gps_data.latitude) / diff_latitude) * metadata.gps_data_rows;

    auto idx = tile_y * metadata.gps_data_row_size + tile_x;

    constexpr auto kScanDistance = 3;
    for (auto dx = -kScanDistance; dx < kScanDistance; dx++)
    {
        for (auto dy = -kScanDistance; dy < kScanDistance; dy++)
        {
            uint32_t cur_index = (tile_y + dy) * metadata.gps_data_row_size + (tile_x + dx);

            if (cur_index >= positions.size())
            {
                return {0, 0};
            }

            // Make a copy to get it out of flash
            auto cur = positions[cur_index];

            if (auto point = PositionIsInTile(cur, tile_x + dx, tile_y + dy, gps_data); point)
            {
                g_current_tile.tile = cur;
                g_current_tile.x = tile_x + dx;
                g_current_tile.y = tile_y + dy;

                return *point;
            }
        }
    }

    return {0, 0};
}

GpsPosition
PointToPosition(const MapMetadata& metadata, const Point& pixel_position)
{
    auto x = pixel_position.x;
    auto y = pixel_position.y;

    auto positions = PositionSpanFromMetadata(metadata);
    auto index = (y / kGpsPositionSize - 1) * metadata.gps_data_row_size + x / kGpsPositionSize;

    if (index >= positions.size())
    {
        // Should never happen
        return GpsPosition {.latitude = 0, .longitude = 0};
    }
    auto pos = positions[index];

    float x_offset = x % kGpsPositionSize;
    float y_offset = y % kGpsPositionSize;

    auto longitude = pos.longitude + (pos.longitude_offset * x_offset) / kGpsPositionSize;
    auto latitude = pos.latitude + (pos.latitude_offset * y_offset) / kGpsPositionSize;

    //    fmt::print("PTP: idx {} ({}x{}), which is {},{}..{},{}. {},{} -> {},{}\n",
    //               index,
    //               index % metadata.gps_data_row_size,
    //               index / metadata.gps_data_row_size,
    //               pos.latitude,
    //               pos.longitude,
    //               pos.latitude + pos.latitude_offset,
    //               pos.longitude + pos.longitude_offset,
    //               x,
    //               y,
    //               longitude,
    //               latitude);
    return GpsPosition {.latitude = latitude, .longitude = longitude};
}

} // namespace gps

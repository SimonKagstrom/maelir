#pragma once

#include "hal/i_gps.hh"
#include "position_converter.hh"
#include "router.hh"
#include "tile.hh"

#include <cmath>
#include <numbers>

static inline IndexType
PointToLandIndex(Point point, unsigned row_size)
{
    return (point.y / kPathFinderTileSize) * row_size + (point.x / kPathFinderTileSize);
}

static inline Point
LandIndexToPoint(IndexType index, unsigned row_size)
{
    return {static_cast<int32_t>((index % row_size) * kPathFinderTileSize),
            static_cast<int32_t>((index / row_size) * kPathFinderTileSize)};
}

static inline bool
IsWater(std::span<const uint32_t> land_mask, IndexType index)
{
    if (index / 32 >= land_mask.size())
    {
        return false;
    }

    return (land_mask[index / 32] & (1 << (index % 32))) == 0;
}

static inline Vector
IndexPairToDirection(IndexType from, IndexType to, unsigned row_size)
{
    int from_x = from % row_size;
    int from_y = from / row_size;
    int to_x = to % row_size;
    int to_y = to / row_size;

    return PointPairToVector({from_x, from_y}, {to_x, to_y});
}

// From https://stackoverflow.com/questions/639695/how-to-convert-latitude-or-longitude-to-meters
static float
Wgs84DeltaToMeters(const GpsPosition& pos1, const GpsPosition& pos2)
{
    auto lat1 = pos1.latitude;
    auto lon1 = pos1.longitude;
    auto lat2 = pos2.latitude;
    auto lon2 = pos2.longitude;

    using namespace std::numbers;

    constexpr auto kPi = std::numbers::pi_v<float>;

    constexpr auto R = 6378.137f; // Radius of earth in KM
    float dLat = lat2 * kPi / 180 - lat1 * kPi / 180;
    float dLon = lon2 * kPi / 180 - lon1 * kPi / 180;
    float a = std::sinf(dLat / 2) * std::sinf(dLat / 2) +
              std::cosf(lat1 * kPi / 180) * std::cosf(lat2 * kPi / 180) * std::sinf(dLon / 2) *
                  std::sinf(dLon / 2);
    float c = 2 * std::atan2f(std::sqrtf(a), std::sqrtf(1 - a));
    float d = R * c;

    return d * 1000.0f; // meters
}

static float
LookupMetersPerPixel(const MapMetadata& metadata)
{
    auto gps_metadata = gps::PositionSpanFromMetadata(metadata);

    for (auto cur : gps_metadata)
    {
        if (cur.latitude_offset != 0 && cur.longitude_offset != 0)
        {
            return Wgs84DeltaToMeters({cur.latitude, cur.longitude},
                                      {cur.latitude + cur.latitude_offset, cur.longitude}) /
                   kGpsPositionSize;
        }
    }

    return 0;
}
